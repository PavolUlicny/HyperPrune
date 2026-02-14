# HyperPrune

> An ultra-optimized, perfect-play Tic-Tac-Toe engine. Uses alpha-beta pruning, bitboards, and a Zobrist-hashed transposition table to achieve millions of self-play games per second on 3×3 boards without any precomputed move tables. Configurable from 3×3 to 8×8, deterministic, and designed for maximum performance.

## Highlights

- Perfect-play AI with full-depth search
- Interactive play and self-play benchmarks
- Deterministic results with optional seeding
- PGO target for peak performance

## Requirements

- C11 compiler (GCC or Clang)
- POSIX environment (Linux, macOS, BSD, WSL)
- Make (optional, recommended)

## Quick start

```sh
make
./ttt
```

## Build

```sh
make              # Default release build
make debug        # Debug build with symbols
make release      # Optimized release build
make pgo          # Profile-guided optimization
```

### Board size (3-8)

```sh
make BOARD_SIZE=4
make pgo BOARD_SIZE=5
```

### Manual build

```sh
gcc -std=c11 -O3 -march=native -flto -DBOARD_SIZE=3 \
  src/main.c src/TicTacToe/tic_tac_toe.c \
  src/MiniMax/mini_max.c src/MiniMax/transposition.c \
  -o ttt -lm
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
--tt-size SIZE, -t SIZE       Transposition table size in entries
--seed SEED                   PRNG seed for Zobrist keys
```

### Examples

```sh
./ttt
./ttt --selfplay 5000
./ttt -s 20000 -q
./ttt --seed 42 -s 1000
./ttt -t 50000000 -s 10000
```

## Testing

The test suite uses Unity and ships with the repo.

```sh
make test
make BOARD_SIZE=4 test
```

The test runner is built at `test/test_runner`.

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
