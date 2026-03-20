# HyperPrune

> The fastest perfect-play Tic-Tac-Toe engine built from scratch — negamax alpha-beta, bitboards, killer moves, history heuristics, and Zobrist-hashed transposition tables, with no precomputed lookup tables. Deterministic perfect play from 3×3 to 8×8.

[![Tests](https://github.com/PavolUlicny/HyperPrune/actions/workflows/test.yml/badge.svg)](https://github.com/PavolUlicny/HyperPrune/actions/workflows/test.yml) [![CodeQL](https://github.com/PavolUlicny/HyperPrune/actions/workflows/codeql.yml/badge.svg)](https://github.com/PavolUlicny/HyperPrune/actions/workflows/codeql.yml) [![Release](https://img.shields.io/github/v/release/PavolUlicny/HyperPrune)](https://github.com/PavolUlicny/HyperPrune/releases/latest) ![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS%20%7C%20Windows-blue)

![Demo](demo.gif)

## Highlights

- Bitboard representation
- Negamax with alpha-beta pruning
- Zobrist-hashed transposition table
- Killer-move and history heuristics for move ordering
- Profile-guided optimization (PGO) target for peak performance
- Verified perfect play on 3×3 and 4×4 (100% ties) across Linux, macOS, and Windows in CI
- ~7.3 M games/s (3×3 release), ~8.2 M games/s (3×3 PGO) on AMD Ryzen 7 8845HS
- Automated CI/CD — comprehensive test matrix (sanitizers, Valgrind, strict warnings, CMake, 6 board sizes) on every push; tag a commit with `v*` to release Linux, macOS, and Windows binaries with SHA256 checksums

## Requirements

- C11 compiler (GCC, Clang, or MSVC)
- Platform: Linux, macOS, Windows (x64), BSD, WSL
- Build system: Make (Unix) or CMake (cross-platform)

> **Note:** Boards 5×5 and above are impractical for real-time play — see [Performance Notes](#performance-notes).

## Quick start

```sh
make
./ttt
```

## Prebuilt binaries

Download the latest release from the [Releases](https://github.com/PavolUlicny/HyperPrune/releases) page.

On Linux and macOS, mark the binary executable before running:

```sh
chmod +x ttt-linux-x86_64   # or ttt-macos-arm64
./ttt-linux-x86_64
```

Windows binaries can be run directly. Released binaries are compiled for 3×3. For other board sizes, compile from source.

## Build

### Unix (Linux, macOS, BSD) - Makefile

```sh
make                   # Default release build
make debug             # Debug build with symbols
make release           # Optimized release build
make portable          # Portable build: no -march=native; statically linked on Linux
make pgo               # Profile-guided optimization
sudo make install      # Install to /usr/local/bin (override with PREFIX=...)
sudo make uninstall    # Remove installed binary
```

Board size (3–8):

```sh
make BOARD_SIZE=4
make pgo BOARD_SIZE=5
```

Move ordering (killer-move and history heuristics):

```sh
make BOARD_SIZE=5 ENABLE_MOVE_ORDERING=0   # Default for BOARD_SIZE >= 5
make BOARD_SIZE=3 ENABLE_MOVE_ORDERING=1   # Default for BOARD_SIZE <= 4
```

### Cross-platform - CMake

```sh
# Linux/macOS (default portable build)
cmake -B build -DBOARD_SIZE=3 -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/ttt

# Linux/macOS (optimized for native CPU)
cmake -B build -DBOARD_SIZE=3 -DCMAKE_BUILD_TYPE=Release -DENABLE_NATIVE_OPTIMIZATIONS=ON
cmake --build build
./build/ttt

# Windows (MSVC)
cmake -B build -DBOARD_SIZE=3 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\Release\ttt.exe
```

**Native optimizations:**

The `ENABLE_NATIVE_OPTIMIZATIONS` CMake option adds `-march=native` to target the build machine's CPU for additional performance. LTO, `-funroll-loops`, and `-fno-semantic-interposition` are always applied to Release builds without this option. Use `ON` for local builds, `OFF` (default) for portable binaries.

**Move ordering:**

The `ENABLE_MOVE_ORDERING` option controls killer-move and history heuristics. Defaults to `ON` for `BOARD_SIZE <= 4` where the pruning gains outweigh the O(n²) sort cost, and `OFF` for larger boards where it degrades performance. Can be overridden: `-DENABLE_MOVE_ORDERING=OFF`. For manual builds that do not pass this flag, the source applies the same default automatically. Note: when changing `BOARD_SIZE` in an existing build directory, the cached `ENABLE_MOVE_ORDERING` value is preserved — use a fresh build directory or pass `-DENABLE_MOVE_ORDERING=` explicitly to get the correct default for the new board size.

### Manual build

```sh
# Unix (GCC/Clang)
gcc -std=c11 -O3 -march=native -flto -DBOARD_SIZE=3 \
  src/main.c src/TicTacToe/tic_tac_toe.c \
  src/Negamax/negamax.c src/Negamax/transposition.c \
  -o ttt -lm

# Windows (MSVC)
cl /std:c11 /O2 /DBOARD_SIZE=3 \
  src\main.c src\TicTacToe\tic_tac_toe.c \
  src\Negamax\negamax.c src\Negamax\transposition.c \
  /Fe:ttt.exe
```

## Usage

### Interactive mode

```sh
./ttt
```

- Choose X or O
- Enter moves as column and row numbers (1-indexed)
- Ctrl+D exits

### Self-play

```sh
./ttt --selfplay [GAMES]
./ttt -s 5000
./ttt -s 10000 -q
```

### CLI options

```text
--help, -h                    Show help and exit
--selfplay, -s [GAMES]        Run self-play mode (default: 1000 games)
--quiet, -q                   Suppress output (requires --selfplay)
--tt-size SIZE, -t SIZE       Transposition table size in entries (0 to disable)
--seed SEED, -S SEED          PRNG seed for Zobrist keys (default: fixed internal seed)
```

### Examples

```sh
./ttt                          # Interactive game
./ttt --selfplay 5000          # Run 5000 self-play games
./ttt --selfplay 10000 -q      # Run 10000 games, quiet output
./ttt -S 42 -s 1000            # Custom Zobrist hash seed
./ttt --tt-size 0 -s 1000      # Benchmark without transposition table
```

## How it works

Here is the full pipeline of a single `getAiMove()` call.

**Board representation** — The board is a `Bitboard` struct containing two `uint64_t` fields, `x_pieces` and `o_pieces`. Each bit encodes one cell: bit index `row * BOARD_SIZE + col`. An X in the top-left corner of a 3×3 board is `x_pieces = 1`; an empty board is two zero words. `BOARD_SIZE` is a compile-time constant (3–8, via `-DBOARD_SIZE=N`); `MAX_MOVES = BOARD_SIZE * BOARD_SIZE`. Cell-level operations (`bitboard_make_move`, `bitboard_unmake_move`) are implemented as simple bitwise OR/AND-NOT updates.

**Move generation** — Legal moves are the empty cells, computed as `empty = ~(x_pieces | o_pieces) & VALID_POSITIONS_MASK`. Each move is extracted by count-trailing-zeros (CTZ64 — `__builtin_ctzll` on GCC/Clang, `_BitScanForward64` on MSVC), which finds the index of the lowest set bit; the bit is then cleared with `empty &= empty - 1`. This loop visits every empty cell without branching over occupied ones.

**Search entry point** — `getAiMove()` handles several short-circuits before the search begins: an invalid `aiPlayer` or overlapping pieces returns `(-1, -1)`; a terminal position (either player has won, or the board is full) returns `(-1, -1)`; an empty board returns the center cell (`BOARD_SIZE/2, BOARD_SIZE/2`) without searching; a board with exactly one empty cell plays it immediately. For all other positions, the killer and history tables are cleared with `memset`, the initial Zobrist hash is computed from the current piece layout via `zobrist_hash()`, a side-to-move adjustment is applied (`zobrist_toggle_turn()` if O is to move at root), and the root move loop begins — iterating empty cells via CTZ and calling `negamax()` with depth 0 for each candidate.

**The negamax loop** — Every call to `negamax()` evaluates from the current player's perspective. Scores are relative: `WIN = 100` means the player-to-move wins; `TIE = 0`. The parent receives `-negamax(child, -beta, -alpha)` — a child score of +100 arrives at the parent as -100. This single recursive function replaces separate min and max layers; the window `(alpha, beta)` is passed as `(-beta, -alpha)` to each child call, and pruning fires when `beta <= alpha`. Alpha-beta reduces the search from O(b^d) toward O(b^(d/2)) nodes in the best case.

**Transposition table integration** — The first operation inside `negamax()` is a probe: `transposition_table_probe(hash, beta, &tt_score)`. Each 16-byte entry stores a 64-bit Zobrist hash, a 16-bit score, a type byte (`EXACT` or `LOWERBOUND`), and an occupied flag. The table is indexed by `hash & transposition_table_mask` (power-of-two size for a bitmask modulo). An `EXACT` entry returns its score immediately; a `LOWERBOUND` entry returns only when its score meets or exceeds beta. On 3×3 the hit rate reaches ~99.9%, so most nodes short-circuit here. After a full node evaluation, the result is written back with an always-replace strategy: `EXACT` when all moves were explored or an unconditional win was found; `LOWERBOUND` when a beta cutoff ended the loop early. The Zobrist hash is updated incrementally — XOR in `zobrist_toggle(hash, bit, player)` per piece placed or removed, plus `zobrist_toggle_turn()` per ply to encode side-to-move. Without the turn key, the same board reached as different players-to-move would collide in the table and return wrong scores.

**Terminal conditions** — After the TT probe and before move generation, `negamax()` calls `boardScore()` to check whether the position is terminal. `boardScore()` checks only the opponent's pieces — the player who just moved — since the current player has not yet placed a piece. It calls `bitboard_has_won()`, which scans all `WIN_MASK_COUNT = 2 × BOARD_SIZE + 2` precomputed masks (N row masks, N column masks, main diagonal, anti-diagonal), testing `(player_pieces & mask) == mask` for each. A full board with no winner is `TIE = 0`. If neither condition holds, the sentinel `NOT_TERMINAL` is returned and the search continues into move generation.

**Move ordering** — Controlled at compile time by `ENABLE_MOVE_ORDERING` (default: `ON` for `BOARD_SIZE <= 4`, `OFF` for larger boards). When enabled, `negamax()` splits move generation into two phases. Phase 1 tries up to two killer moves — bit indices stored in `killers[depth][0..1]` from moves that caused cutoffs at this depth in an earlier branch. Phase 2 iterates the remaining empty squares in descending `history[bit]` order, where `history` accumulates the beta-cutoff count each move has caused anywhere in the tree. The Phase 2 selection is a linear scan (O(n²) across all iterations, n ≤ MAX_MOVES). Together, killers and history push likely-cutoff moves to the front, which lets alpha-beta prune more of the tree before it is evaluated. On larger boards the O(n²) sort cost dominates because TT coverage drops and the move loop runs far more often, so ordering is disabled by default.

**Profile-guided optimization** — `make pgo` runs a three-step build: compile with profiling instrumentation, run a self-play workload to collect a real execution profile, then rebuild with that profile fed back to the compiler. The search logic is identical; PGO uses the observed hot paths to guide inlining decisions and branch-prediction hints.

## Testing

The test suite uses Unity and ships with the repo.

**Makefile (Unix):**

```sh
make test
make BOARD_SIZE=4 test
```

**CMake (cross-platform):**

```sh
cmake -B build -DBOARD_SIZE=3 -DCMAKE_BUILD_TYPE=Release
cmake --build build
ctest --test-dir build --output-on-failure
```

The test runner is built at `test/test_runner` (Makefile) or `build/test_runner` (CMake).

## API usage (library-style)

The engine can be used directly from the public headers:

- `TicTacToe/tic_tac_toe.h` for board state, move helpers, and win checks
- `Negamax/negamax.h` for `getAiMove()`
- `Negamax/transposition.h` for Zobrist + transposition table

Minimal init and loop (0-based coordinates):

```c
#include "TicTacToe/tic_tac_toe.h"
#include "Negamax/negamax.h"
#include "Negamax/transposition.h"

int main(void)
{
    init_win_masks();
    zobrist_init();
    transposition_table_init(100000);

    Bitboard board = {0, 0};
    int row = -1, col = -1;
    getAiMove(board, 'x', &row, &col);

    transposition_table_free();
    return 0;
}
```

Compile with `-Isrc` to include the headers:

```sh
gcc -std=c11 -Isrc -DBOARD_SIZE=3 your_program.c \
  src/TicTacToe/tic_tac_toe.c \
  src/Negamax/negamax.c \
  src/Negamax/transposition.c \
  -o your_program -lm
```

Notes:

- Call `zobrist_set_seed()` before `zobrist_init()` if you want a custom seed.
- `getAiMove()` returns `(-1, -1)` on terminal positions.
- `BOARD_SIZE` is compile-time; it must match across all objects.

## Performance notes

Benchmarked on AMD Ryzen 7 8845HS (performance governor):

| Board | Release        | PGO            |
|-------|----------------|----------------|
| 3×3   | ~7.3 M games/s | ~8.2 M games/s |
| 4×4   | ~2.3 M games/s | ~2.8 M games/s |

- Fastest build: `make pgo`
- Released binaries are compiled for 3×3. For other board sizes, compile from source with `make BOARD_SIZE=N`
- Large boards grow exponentially: 5×5 takes on the order of minutes per move; 6×6+ is impractical without further pruning improvements
- Default transposition table sizing is automatic; override with `--tt-size`

## Project structure

```text
src/
├── main.c                    # Entry point and CLI
├── TicTacToe/                # Board logic and I/O helpers
└── Negamax/                  # Search and transposition table
test/
└── unity/                    # Unity test framework
```

## Changelog

See [CHANGELOG.md](CHANGELOG.md) for release history.

## Author

Built by [Pavol Ulicny](https://github.com/PavolUlicny)

## License

MIT License - see [LICENSE](LICENSE).
