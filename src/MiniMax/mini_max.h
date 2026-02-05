#ifndef MINI_MAX_H
#define MINI_MAX_H

/*
 * MiniMax (Tic-Tac-Toe)
 * ---------------------
 * Public API for the Minimax-based Tic-Tac-Toe engine.
 *
 * The engine searches the full game tree using Minimax with alpha–beta pruning
 * and a lightweight move ordering heuristic. It is designed for small boards
 * (e.g., 3x3) but supports arbitrary square sizes via BOARD_SIZE.
 *
 * Board representation:
 * - A 2D char array sized [BOARD_SIZE][BOARD_SIZE]
 * - 'x' and 'o' represent players; ' ' (space) represents an empty cell
 *
 * Notable characteristics:
 * - Deterministic results due to stable ordering within buckets
 * - Depth-adjusted scoring prefers faster wins and delays losses
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
 *  - board:     Current position; modified temporarily during search but restored before return
 *  - aiPlayer:  The AI symbol ('x' or 'o') to maximize for
 *  - out_row:   Output pointer for selected row (0-based). Set to -1 if the game is already terminal
 *  - out_col:   Output pointer for selected column (0-based). Set to -1 if the game is already terminal
 *
 * Behavior:
 *  - If the board is terminal (win/tie), returns (-1, -1)
 *  - On an empty board, selects the center without searching
 *  - Otherwise, orders candidate moves and runs a full-depth alpha–beta search
 */
void getAiMove(char board[BOARD_SIZE][BOARD_SIZE], char aiPlayer, int *out_row, int *out_col);

#ifdef __cplusplus
}
#endif

#endif
