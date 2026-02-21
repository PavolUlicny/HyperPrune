# HyperPrune

> The fastest single-threaded perfect-play Tic-Tac-Toe engine without precomputed lookup tables. Clean API and CLI for interactive play or integration. Alpha-beta pruning, bitboards, and Zobrist-hashed transposition tables deliver deterministic perfect play from 3×3 to 8×8.

## Highlights

- Perfect-play AI with full-depth search
- Interactive play and self-play benchmarks
- Deterministic results with optional seeding
- PGO target for peak performance

## Requirements

- C11 compiler (GCC, Clang, or MSVC)
- Platform: Linux, macOS, Windows (x64), BSD, WSL
- Build system: Make (Unix) or CMake (cross-platform)

## Quick start

```sh
make
./ttt
```

## Build

### Unix (Linux, macOS, BSD) - Makefile

```sh
make              # Default release build
make debug        # Debug build with symbols
make release      # Optimized release build
make pgo          # Profile-guided optimization
```

Board size (3-8):

```sh
make BOARD_SIZE=4
make pgo BOARD_SIZE=5
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

The `ENABLE_NATIVE_OPTIMIZATIONS` CMake option enables aggressive optimizations (`-march=native`, `-flto`, `-funroll-loops`, etc.) for maximum performance on the build machine. Use `ON` for local builds, `OFF` (default) for portable binaries.

### Manual build

```sh
# Unix (GCC/Clang)
gcc -std=c11 -O3 -march=native -flto -DBOARD_SIZE=3 \
  src/main.c src/TicTacToe/tic_tac_toe.c \
  src/MiniMax/mini_max.c src/MiniMax/transposition.c \
  -o ttt -lm

# Windows (MSVC)
cl /std:c11 /O2 /DBOARD_SIZE=3 \
  src\main.c src\TicTacToe\tic_tac_toe.c \
  src\MiniMax\mini_max.c src\MiniMax\transposition.c \
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
--quiet, -q                   Suppress all output in self-play mode
--tt-size SIZE, -t SIZE       Transposition table size in entries (0 to disable)
--seed SEED                   PRNG seed for Zobrist keys
```

### Examples

```sh
./ttt
./ttt --selfplay 5000
./ttt -s 20000 -q
./ttt --seed 42 -s 1000
./ttt -t 50000000 -s 10000
./ttt -t 0 -s 1000              # Benchmark without TT
```

## Testing

The test suite uses Unity and ships with the repo.

**Makefile (Unix):**

```sh
make test
make BOARD_SIZE=4 test
```

**CMake (cross-platform):**

```sh
cmake -B build -DBOARD_SIZE=3
cmake --build build
ctest --test-dir build --output-on-failure
```

The test runner is built at `test/test_runner` (Makefile) or `build/test_runner` (CMake).

## API usage (library-style)

The engine can be used directly from the public headers:

- `TicTacToe/tic_tac_toe.h` for board state, move helpers, and win checks
- `MiniMax/mini_max.h` for `getAiMove()`
- `MiniMax/transposition.h` for Zobrist + transposition table

Minimal init and loop (0-based coordinates):

```c
#include "TicTacToe/tic_tac_toe.h"
#include "MiniMax/mini_max.h"
#include "MiniMax/transposition.h"

int main(void)
{
    init_win_masks();
    zobrist_init();
    transposition_table_init(100000);

    Bitboard board = {0};
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
  src/MiniMax/mini_max.c \
  src/MiniMax/transposition.c \
  -o your_program -lm
```

Notes:

- Call `zobrist_set_seed()` before `zobrist_init()` if you want a custom seed.
- `getAiMove()` returns `(-1, -1)` on terminal positions.
- `BOARD_SIZE` is compile-time; it must match across all objects.

## Performance notes

- Fastest build: `make pgo`
- Large boards (5x5+) grow quickly in search time
- Default transposition table sizing is automatic; override with `--tt-size`

## Project structure

```text
src/
├── main.c                    # Entry point and CLI
├── TicTacToe/                # Board logic and I/O helpers
└── MiniMax/                  # Search and transposition table
test/
└── unity/                    # Unity test framework
```

## License

MIT License - see [LICENSE](LICENSE).
