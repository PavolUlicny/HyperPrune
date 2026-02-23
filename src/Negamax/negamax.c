/*
 * Negamax search with alpha–beta pruning for Tic-Tac-Toe
 * -------------------------------------------------------
 *
 * This file implements a deterministic Negamax engine with:
 *  - Alpha–beta pruning
 *  - Terminal-only scoring (win/loss/tie evaluation)
 *  - Simple opening heuristic: play center on empty board
 *  - Transposition table with Zobrist hashing for position caching
 *
 * Negamax formulation: every node is always from the current player's
 * perspective. The parent receives -negamax(child, -beta, -alpha), so scores
 * are relative to the player-to-move rather than a fixed "AI player".
 *
 * Public entry point: getAiMove(...)
 */

#include "mini_max.h"
#include "transposition.h"
#include "bitops.h"
#include <stdint.h>

/* Helper constants used by the evaluation and search. */
typedef enum
{
    WIN = 100, /* Current player wins */
    TIE_SCORE = 0,
    CONTINUE_SCORE = 1,
    INF = 101
} HelperScores;

/* Compile-time validation: terminal scores must fit in int16_t (transposition table storage) */
_Static_assert(WIN <= INT16_MAX && (-WIN) >= INT16_MIN, "WIN and -WIN must fit in int16_t");

/*
 * Safe mask for valid board positions.
 * Avoids undefined behavior when MAX_MOVES == 64 (1ULL << 64 is UB).
 */
#if MAX_MOVES == 64
static const uint64_t VALID_POSITIONS_MASK = ~0ULL;
#else
static const uint64_t VALID_POSITIONS_MASK = (1ULL << MAX_MOVES) - 1;
#endif

/*
 * Terminal evaluation from currentPlayer's perspective.
 * Called at the start of a node before currentPlayer moves.
 * Only the PREVIOUS player (opponent of currentPlayer) can have just won —
 * currentPlayer hasn't moved yet — so only one bitboard_has_won call suffices.
 *
 *  -WIN (−100)     : opponent of currentPlayer won → currentPlayer loses
 *   TIE_SCORE (0)  : board is full → draw
 *   CONTINUE_SCORE : game not yet terminal
 */
static inline int boardScore(Bitboard board, char currentPlayer)
{
    /* Check if previous player (opponent of currentPlayer) has won */
    uint64_t opp_pieces = (currentPlayer == 'x') ? board.o_pieces : board.x_pieces;
    if (bitboard_has_won(opp_pieces))
        return -WIN;

    /* Check if board is full (tie) */
    uint64_t occupied = board.x_pieces | board.o_pieces;
    if (occupied == VALID_POSITIONS_MASK)
        return TIE_SCORE;

    return CONTINUE_SCORE;
}

/*
 * Negamax search with alpha-beta pruning.
 * Returns the best score achievable for currentPlayer from the current position.
 * All scores are relative to currentPlayer (positive = good for currentPlayer).
 */
static int negamax(Bitboard board, char currentPlayer, int alpha, int beta, uint64_t hash)
{
    /* Transposition table probe */
    int tt_score;
    if (transposition_table_probe(hash, beta, &tt_score))
        return tt_score;

    /* Terminal check: did the previous player win? */
    int state = boardScore(board, currentPlayer);
    if (state != CONTINUE_SCORE)
    {
        /* Terminal: cache and return exact score */
        transposition_table_store(hash, state, TRANSPOSITION_TABLE_EXACT);
        return state;
    }

    char opponent = (currentPlayer == 'x') ? 'o' : 'x';
    uint64_t empty = ~(board.x_pieces | board.o_pieces) & VALID_POSITIONS_MASK;
    int bestScore = -INF;

    while (empty)
    {
        int bit = CTZ64(empty);
        empty &= empty - 1;
        int row = BIT_TO_ROW(bit);
        int col = BIT_TO_COL(bit);
        bitboard_make_move(&board, row, col, currentPlayer);
        uint64_t new_hash = zobrist_toggle(hash, row, col, currentPlayer);
        new_hash = zobrist_toggle_turn(new_hash);
        int score = -negamax(board, opponent, -beta, -alpha, new_hash);
        bitboard_unmake_move(&board, row, col, currentPlayer);

        if (score > bestScore)
            bestScore = score;

        /* Early exit: current player wins, can't do better */
        if (bestScore == WIN)
            break;

        if (score > alpha)
            alpha = score;
        if (beta <= alpha)
            break; /* Beta cutoff */
    }

    /* Classify node type for transposition table storage.
     * Only two early exits exist: WIN (absolute maximum) and beta cutoff
     * (beta <= alpha). There is no alpha-cutoff exit that would leave moves
     * unexplored, so bestScore is always the true value when no beta cutoff
     * occurred. UPPERBOUND is therefore never needed.
     *
     * WIN is the absolute maximum — always EXACT regardless of beta.
     * LOWERBOUND when bestScore >= beta (beta cutoff; true value >= bestScore).
     * EXACT in all other cases (all moves explored; bestScore is the true value). */
    TranspositionTableNodeType store_type;
    if (bestScore == WIN || bestScore < beta)
        store_type = TRANSPOSITION_TABLE_EXACT;
    else
        store_type = TRANSPOSITION_TABLE_LOWERBOUND;
    transposition_table_store(hash, bestScore, store_type);

    return bestScore;
}

/*
 * Public entry: select the best move for aiPlayer.
 * Short-circuits:
 *  - Invalid board (overlapping pieces) -> (-1, -1)
 *  - Terminal board -> (-1, -1)
 *  - Empty board    -> center (BOARD_SIZE/2, BOARD_SIZE/2) without searching
 */
void getAiMove(Bitboard board, char aiPlayer, int *out_row, int *out_col)
{
    /* Validate: no overlapping pieces */
    if (board.x_pieces & board.o_pieces)
    {
        *out_row = -1;
        *out_col = -1;
        return;
    }

    /* Terminal check (both players — either may have already won) */
    if (bitboard_has_won(board.x_pieces) || bitboard_has_won(board.o_pieces) ||
        (board.x_pieces | board.o_pieces) == VALID_POSITIONS_MASK)
    {
        *out_row = -1;
        *out_col = -1;
        return;
    }

    uint64_t empty = ~(board.x_pieces | board.o_pieces) & VALID_POSITIONS_MASK;

    if (empty == VALID_POSITIONS_MASK)
    {
        /* Empty board: play center (lower-right of central 2×2 for even boards) */
        *out_row = BOARD_SIZE / 2;
        *out_col = BOARD_SIZE / 2;
        return;
    }

    if ((empty & (empty - 1)) == 0)
    {
        /* Only one empty cell: play it immediately */
        int bit = CTZ64(empty);
        *out_row = BIT_TO_ROW(bit);
        *out_col = BIT_TO_COL(bit);
        return;
    }

    int alpha = -INF;
    int beta = INF;
    int bestRow = -1;
    int bestCol = -1;
    int bestScore = -INF;
    uint64_t hash = zobrist_hash(board);
    char opponent = (aiPlayer == 'x') ? 'o' : 'x';

    while (empty)
    {
        int bit = CTZ64(empty);
        empty &= empty - 1;
        int row = BIT_TO_ROW(bit);
        int col = BIT_TO_COL(bit);
        bitboard_make_move(&board, row, col, aiPlayer);
        uint64_t new_hash = zobrist_toggle(hash, row, col, aiPlayer);
        new_hash = zobrist_toggle_turn(new_hash);
        int score = -negamax(board, opponent, -beta, -alpha, new_hash);
        bitboard_unmake_move(&board, row, col, aiPlayer);

        if (score > bestScore)
        {
            bestScore = score;
            bestRow = row;
            bestCol = col;
            alpha = score;
        }

        /* Early exit: found a winning move */
        if (bestScore == WIN)
            break;
    }

    *out_row = bestRow;
    *out_col = bestCol;
}
