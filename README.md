# MiniMax Tic-Tac-Toe

A compact, algorithm-focused Tic-Tac-Toe engine implemented in C featuring a full-depth Minimax search with alpha–beta pruning, transposition table caching, and targeted move ordering. The board size is configurable via `BOARD_SIZE` (default 3). On a 3×3 board the engine plays perfectly — self-play always ends in a draw.

- Language: C
- Build system: Make
- License: MIT

## Minimax-first overview

Core search is implemented with two mutually recursive functions representing the maximizing and minimizing plies:

- `miniMaxHigh(board, aiPlayer, depth, alpha, beta, hash)`: maximizing side (AI)
- `miniMaxLow(board, aiPlayer, depth, alpha, beta, hash)`: minimizing side (opponent)

Both return an integer score. Constants used throughout the engine (see `src/MiniMax/mini_max.c`):

- `AI_WIN_SCORE = 100`, `PLAYER_WIN_SCORE = -100`
- `TIE_SCORE = 0`
- `CONTINUE_SCORE = 1` (sentinel: game not terminal)
- `INF = INT_MAX`

Depth-based scoring tweaks terminal values to prefer quicker wins and delay losses:

- AI win at depth d → `AI_WIN_SCORE - d`
- AI loss at depth d → `PLAYER_WIN_SCORE + d`

### Optimizations that matter

- **Transposition table with Zobrist hashing** (SYSTEMATICALLY OPTIMIZED)
  - Caches position evaluations to avoid re-searching identical board states reached via different move orders.
  - Uses 64-bit Zobrist hashing for fast incremental position identification.
  - Stores exact scores, lower bounds (beta cutoffs), and upper bounds (alpha cutoffs) with depth information.
  - Always-replace strategy for hash collisions (table sized for near-zero collision rates).
  - **Dynamic sizing**: Automatically calculates optimal table size for any board size
    - Formula: `size = 1.5M × (BOARD_SIZE / 4)^9.4` (derived from empirical testing)
    - 3×3: 100K entries (1.5 MB) - verified optimal with 100M games
    - 4×4: 1.5M entries (24 MB) - verified optimal with 5M games
    - 5×5: ~12M entries (187 MB) - extrapolated for max performance
    - 6×6: ~68M entries (1 GB) - extrapolated for max performance
    - 7×7+: Capped at 100M entries (1.6 GB) - configurable via `MAX_TRANSPOSITION_TABLE_SIZE`
  - **Methodology**: Systematic small/medium/large range testing across 644M games (29 configurations)
  - **Performance**: 1,838,462x speedup on 4×4 vs no table (0.13 → 239K games/s)
  - **Hit rates**: 100.0% on both 3×3 and 4×4 (perfect caching, zero misses)
  - **Peak throughput**: 2.21M games/s on 3×3, 239K games/s on 4×4

- Alpha–beta pruning
  - Each node tracks `(alpha, beta)`; prune when `beta <= alpha`.

- Targeted move ordering
  - `moveWeight(row, col)` partitions moves into buckets before searching:
  - Weight 4: center (distance 0 from center; for even boards this yields one of the four central squares).
  - Weight 3: diagonal squares OR Manhattan distance 1 from center.
  - Weight 2: remaining squares.
  - `orderMoves(...)` concatenates the buckets (4 → 3 → 2). Within a bucket, original generation order (row-major) is preserved, making the AI deterministic when scores tie.

- Early cutoffs
  - After making a move, `didLastMoveWin(...)` short-circuits to a terminal score without deeper recursion.
  - If only one empty square remains, return `TIE_SCORE` immediately.
  - `boardScore(...)` quickly detects row/column/diagonal wins and tie/full-board states; otherwise returns `CONTINUE_SCORE`.

- Opening heuristic
  - On an empty board, `getAiMove(...)` plays the center without searching. For even-sized boards, it picks the square at indices `(BOARD_SIZE/2, BOARD_SIZE/2)` (0-based), i.e., the lower-right of the central 2×2.

### Public function highlights

- `void getAiMove(char board[BOARD_SIZE][BOARD_SIZE], char aiPlayer, int* out_row, int* out_col)`
  - Returns `(-1, -1)` if the position is already terminal (win or tie) for either side.
  - Otherwise, orders moves and runs a full-depth alpha–beta search (first reply via `miniMaxLow`) to pick the best move. If a top-level immediate win is found, it is returned directly.

- `static int boardScore(const char board[...], char aiPlayer)`
  - Evaluates only for terminal detection: returns `AI_WIN_SCORE`/`PLAYER_WIN_SCORE` based on who completed a line relative to `aiPlayer`, `TIE_SCORE` if full and no winner, or `CONTINUE_SCORE` when moves remain.

## Key sources

- Engine: [`src/MiniMax/mini_max.c`](src/MiniMax/mini_max.c), [`src/MiniMax/mini_max.h`](src/MiniMax/mini_max.h)
- Transposition table: [`src/MiniMax/transposition.c`](src/MiniMax/transposition.c), [`src/MiniMax/transposition.h`](src/MiniMax/transposition.h)
- Game/UI scaffolding: [`src/TicTacToe/tic_tac_toe.c`](src/TicTacToe/tic_tac_toe.c), [`src/TicTacToe/tic_tac_toe.h`](src/TicTacToe/tic_tac_toe.h)
- Entry point & self-play: [`src/main.c`](src/main.c)
- Build: [`Makefile`](Makefile)

## Build and run

Requirements:

- GCC or Clang compatible with C11
- Linux, macOS, or Windows (MinGW)

Common targets:

- Build (release by default): `make`
- Run: `make run`
- Debug build: `make debug`
- Release build: `make release`
- Clean: `make clean`

Release build flags include:

- `-O3 -march=native -flto -fomit-frame-pointer -DNDEBUG`

### Build without Make (manual)

You can compile directly with gcc or clang. The commands below produce the same `ttt` binary name as the Makefile.

- Release (gcc):

```sh
gcc -std=c11 -Wall -Wextra -O3 -march=native -flto -fomit-frame-pointer -DNDEBUG -pipe \
  src/main.c src/TicTacToe/tic_tac_toe.c src/MiniMax/mini_max.c src/MiniMax/transposition.c -o ttt
```

- Debug (gcc):

```sh
gcc -std=c11 -Wall -Wextra -O0 -g -pipe \
  src/main.c src/TicTacToe/tic_tac_toe.c src/MiniMax/mini_max.c src/MiniMax/transposition.c -o ttt
```

- Using clang: replace `gcc` with `clang`.

Run the binary:

```sh
./ttt
```

### Change board size

`BOARD_SIZE` defaults to 3. Override it at compile time via Make:

```sh
make BOARD_SIZE=4
```

…or by editing the default in [`tic_tac_toe.h`](src/TicTacToe/tic_tac_toe.h).

Note: The search space grows exponentially with board size. The transposition table provides dramatic speedup for larger boards (500x+ on 4×4), making boards up to 4×4 practical for self-play benchmarking.

To override `BOARD_SIZE` without Make, pass `-DBOARD_SIZE=4` (example) to the compile command, e.g.:

```sh
gcc -std=c11 -Wall -Wextra -O3 -march=native -flto -fomit-frame-pointer -DNDEBUG -pipe \
  -DBOARD_SIZE=4 \
  src/main.c src/TicTacToe/tic_tac_toe.c src/MiniMax/mini_max.c src/MiniMax/transposition.c -o ttt
```

## CLI usage

Interactive game:

- Run the compiled `ttt` binary (or `make run`) and follow prompts to play as X or O.

Self-play benchmark mode:

- `--selfplay [games] [--quiet]`
  - Example: `./ttt -s 10000 -q`
  - Without `--quiet`, timing and throughput (games/s) are printed.
  - Short flags are supported: `-s` for `--selfplay`, `-q` for `--quiet`.
  - If `[games]` is omitted, the default is `1000`.
  - On a 3×3 board all games end in ties — this is the expected result for optimal play.

## Using the engine

Minimal example:

```c
#include "TicTacToe/tic_tac_toe.h"
#include "MiniMax/mini_max.h"
#include "MiniMax/transposition.h"

int main(void)
{
    // Initialize transposition table (call once at program start)
    zobrist_init();
    transposition_table_init(1000000);  // 1M entries (~16 MB)

    char board[BOARD_SIZE][BOARD_SIZE];
    initializeBoard();

    // ... populate board with current position ...

    int r = -1, c = -1;
    getAiMove(board, /* aiPlayer */ 'x', &r, &c);
    // If r,c are -1,-1 the position was terminal; otherwise play (r,c)

    // Clean up at program exit
    transposition_table_free();
    return 0;
}
```

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.
