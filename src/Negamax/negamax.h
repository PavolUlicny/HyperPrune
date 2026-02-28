#ifndef NEGAMAX_H
#define NEGAMAX_H

/*
 * Negamax (Tic-Tac-Toe)
 * ---------------------
 * Public API for the Negamax-based Tic-Tac-Toe engine.
 *
 * The engine searches the full game tree using Negamax with alpha–beta pruning.
 * BOARD_SIZE is configured at compile time (3–8, per tic_tac_toe.h).
 *
 * Board representation:
 * - Bitboard structure with two uint64_t (x_pieces, o_pieces)
 * - 'x' and 'o' pieces stored as bit positions (max 8x8 = 64 bits)
 *
 * Notable characteristics:
 * - Deterministic results due to stable ordering of move generation
 * - Simple opening heuristic (play center on empty board)
 * - Transposition table with Zobrist hashing for position caching
 * - Killer move heuristic: try refutation moves first at each depth
 * - History heuristic: order remaining moves by cumulative beta-cutoff count
 */

#include "../TicTacToe/tic_tac_toe.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Compute the AI's next move using Negamax with alpha–beta pruning.
     *
     * Prerequisite: init_win_masks() must be called once before the first call.
     *
     * Parameters:
     *  - board:     Current position (bitboard representation)
     *  - aiPlayer:  The AI symbol ('x' or 'o') to maximize for
     *  - out_row:   Output pointer for selected row (0-based). Set to -1 if the game is already terminal
     *  - out_col:   Output pointer for selected column (0-based). Set to -1 if the game is already terminal
     *
     * Behavior:
     *  - If the board is invalid (x_pieces & o_pieces != 0), returns (-1, -1)
     *  - If the board is terminal (win/tie), returns (-1, -1)
     *  - On an empty board, selects the center without searching
     *  - Otherwise, runs a full-depth alpha–beta search
     *  - Killer and history tables are reset at the start of each call; they do
     *    not persist across calls (repeated calls on the same position are deterministic)
     */
    void getAiMove(Bitboard board, char aiPlayer, int *out_row, int *out_col);

#ifdef __cplusplus
}
#endif

#endif
