# HyperPrune

A highly optimized Tic-Tac-Toe engine with perfect-play AI. Features aggressive alpha-beta pruning, bitboard representation, Zobrist-hashed transposition table, and configurable board sizes (3×3 to 8×8).

## Features

- Interactive mode: play against a perfect AI
- Self-play mode: run AI vs AI simulations with performance metrics
- Deterministic gameplay with optional PRNG seeding
- Profile-guided optimization (PGO) support for maximum performance

## Requirements

- C11 compiler (GCC or Clang)
- POSIX environment (Linux, macOS, BSD, WSL)
- Make (optional but recommended)

## Quick Start

```sh
make              # Build with default settings (3×3 board)
./ttt             # Start interactive game
```

## Build

### Standard builds

```sh
make              # Default release build
make debug        # Debug build with symbols
make release      # Optimized release build
make pgo          # Profile-guided optimization (fastest)
```

### Custom board size

Build with `BOARD_SIZE` between 3 and 8:

```sh
make BOARD_SIZE=4              # 4×4 board
make pgo BOARD_SIZE=5          # 5×5 with PGO
```

### Manual build (without Make)

Using GCC:

```sh
gcc -std=c11 -O3 -march=native -flto -DBOARD_SIZE=3 \
  src/main.c src/TicTacToe/tic_tac_toe.c \
  src/MiniMax/mini_max.c src/MiniMax/transposition.c \
  -o ttt -lm
```

Using Clang:

```sh
clang -std=c11 -O3 -march=native -flto -DBOARD_SIZE=3 \
  src/main.c src/TicTacToe/tic_tac_toe.c \
  src/MiniMax/mini_max.c src/MiniMax/transposition.c \
  -o ttt -lm
```

## Usage

### Interactive mode

Play against the AI:

```sh
./ttt
```

The game will prompt you to:

1. Choose your symbol (X or O)
2. Enter moves as column and row numbers (1-indexed)
3. Press Ctrl+D to quit anytime

### Self-play mode

Run AI vs AI simulations:

```sh
./ttt --selfplay [GAMES]      # Run simulations (default: 1000 games)
./ttt -s 5000                 # Run 5000 games with full statistics
./ttt -s 10000 -q             # Run 10000 games quietly
```

### Command-line options

```text
--help, -h                    Show help and exit
--selfplay, -s [GAMES]        Run self-play mode (default: 1000 games)
--quiet, -q                   Suppress all output in self-play mode
--tt-size SIZE, -t SIZE       Set transposition table size (max: 100000000)
--seed SEED                   Set PRNG seed for Zobrist keys
```

### Examples

```sh
./ttt                                    # Interactive game
./ttt --help                             # Show all options
./ttt --selfplay 5000                    # 5000 games with statistics
./ttt -s 20000 -q                        # 20000 games, no output
./ttt --seed 42 -s 1000                  # Deterministic run with seed 42
./ttt -t 50000000 -s 10000               # Custom transposition table size
```

## Performance

- **Fastest build**: `make pgo` (profile-guided optimization)
- **Portability**: Remove `-march=native` for portable binaries
- **Larger boards**: 5×5+ dramatically increase search time
- **3×3 performance**: 100,000+ games/second typical on modern CPUs

## How it works

- **Board representation**: Dual bitboards (64-bit integers)
- **Search algorithm**: Full-depth minimax with alpha-beta pruning
- **Evaluation**: Terminal-only scoring (no heuristics)
- **Caching**: Zobrist-hashed transposition table with node type bounds
- **Determinism**: Stable move ordering ensures consistent results

## Project structure

```text
src/
├── main.c                    # Entry point and CLI
├── TicTacToe/                # Game logic and board management
└── MiniMax/                  # Search algorithm and transposition table
```

## License

MIT License - see [LICENSE](LICENSE) file for details.
