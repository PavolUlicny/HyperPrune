# MiniMax Tic-Tac-Toe (C)

An extremely optimised deterministic Tic-Tac-Toe engine and CLI written in C. The game uses a compact bitboard representation and a full-depth minimax search with alpha-beta pruning. A Zobrist-hashed transposition table caches repeated positions to reduce work. Board size is compile-time configurable from 3x3 up to 8x8.

- Language: C11
- Build system: Make
- License: MIT

## What this project does

This repository provides:

- A terminal UI for human vs AI play and AI self-play runs
- A minimax engine with alpha-beta pruning and deterministic move ordering
- A transposition table backed by Zobrist hashing
- Bitboard utilities for fast win detection and move generation

The engine evaluates terminal states only (win/loss/tie). On an empty board it plays the center square without searching.

## Build

Requirements: GCC or Clang with C11 support and a POSIX shell.

## Platform constraints

- Designed for Unix-like environments (Linux, macOS, *BSD) with a POSIX shell.
- Requires a C11 compiler (GCC or Clang).
- Windows support depends on a POSIX-compatible environment (for example WSL or MSYS2); the Makefile is not guaranteed to work in plain `cmd.exe` or PowerShell.

```sh
make
```

Common targets:

- `make run`
- `make debug`
- `make release`
- `make pgo`
- `make clean`

Note: `make pgo` enables profile-guided optimization and typically yields a significant speedup.

## Performance notes

- `make pgo` is the fastest build in most cases.
- `-march=native` enables CPU-specific optimizations; remove it for portable binaries.
- Larger boards (5x5 and up) expand the search space quickly.

## Manual build (no Make)

If you do not have `make`, compile the sources directly. Example using GCC:

```sh
gcc -std=c11 -Wall -Wextra -O3 -march=native -flto -fomit-frame-pointer -DNDEBUG -pipe \
  -DBOARD_SIZE=3 \
  src/main.c src/TicTacToe/tic_tac_toe.c src/MiniMax/mini_max.c src/MiniMax/transposition.c -o ttt -lm
```

Example using Clang:

```sh
clang -std=c11 -Wall -Wextra -O3 -march=native -flto -fomit-frame-pointer -DNDEBUG -pipe \
  -DBOARD_SIZE=3 \
  src/main.c src/TicTacToe/tic_tac_toe.c src/MiniMax/mini_max.c src/MiniMax/transposition.c -o ttt -lm
```

## Run

Interactive (human vs AI):

```sh
./ttt
```

Self-play (AI vs AI):

```sh
./ttt --selfplay 10000 --quiet
```

CLI flags:

- `--selfplay`, `-s` optional game count (default 1000)
- `--quiet`, `-q` suppress all self-play output (outcomes, timing, and transposition table stats)
- `--tt-size SIZE`, `-t SIZE` override transposition table size in entries (default: auto-sized, max 100M)
- `--seed SEED` set PRNG seed for Zobrist key generation (default: deterministic)

CLI examples:

```sh
./ttt --selfplay 5000
./ttt --selfplay 20000 --quiet
./ttt --seed 12345 --selfplay 1000
```

## Configure board size

`BOARD_SIZE` defaults to 3 and must be between 3 and 8.

```sh
make BOARD_SIZE=4
```

## Engine details

- Board representation: two `uint64_t` bitboards (`x` and `o`), supporting up to 64 cells
- Move generation: bit scanning of empty squares
- Search: full-depth minimax with alpha-beta pruning
- Evaluation: terminal-only scoring (win/loss/tie)
- Determinism: stable move ordering and consistent tie-breaking
- Transposition table: Zobrist keys (SplitMix64 PRNG), node type storage (exact, lower, upper)

## Project layout

- `src/main.c` entry point and CLI modes
- `src/TicTacToe/` game state, board I/O, win detection, bitboard helpers
- `src/MiniMax/` minimax engine and transposition table implementation

## License

MIT. See [LICENSE](LICENSE).
