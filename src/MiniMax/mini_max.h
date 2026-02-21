#ifndef MINI_MAX_H
#define MINI_MAX_H

/*
 * MiniMax (Tic-Tac-Toe)
 * ---------------------
 * Public API for the Minimax-based Tic-Tac-Toe engine.
 *
 * The engine searches the full game tree using Minimax with alpha–beta pruning.
 * BOARD_SIZE is configured at
 * compile time (3-8 per tic_tac_toe.h).
 *
 * Board representation:
 * - Bitboard structure with two uint64_t (x_pieces, o_pieces)
 * - 'x' and 'o' pieces stored as bit positions (max 8x8 = 64 bits)
 *
 * Notable characteristics:
 * - Deterministic results due to stable ordering of move generation
 * - Simple opening heuristic (play center on empty board)
 */

#include "../TicTacToe/tic_tac_toe.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
     * Compute the AI's next move using Minimax with alpha–beta pruning.
     *
     * Parameters:
     *  - board:     Current position (bitboard representation)
     *  - aiPlayer:  The AI symbol ('x' or 'o') to maximize for
     *  - out_row:   Output pointer for selected row (0-based). Set to -1 if the game is already terminal
     *  - out_col:   Output pointer for selected column (0-based). Set to -1 if the game is already terminal
     *
     * Behavior:
     *  - If the board is terminal (win/tie), returns (-1, -1)
     *  - On an empty board, selects the center without searching
     *  - Otherwise, runs a full-depth alpha–beta search
     */
    void getAiMove(Bitboard board, char aiPlayer, int *out_row, int *out_col);

#ifdef __cplusplus
}
#endif

#endif
