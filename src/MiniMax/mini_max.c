/*
 * Minimax search with alpha–beta pruning for Tic-Tac-Toe
 * -------------------------------------------------------
 *
 * This file implements a deterministic Minimax engine with:
 *  - Alpha–beta pruning
 *  - Terminal-only scoring (win/loss/tie evaluation)
 *  - Simple opening heuristic: play center on empty board
 *  - Transposition table with Zobrist hashing for position caching
 *
 * Public entry point: getAiMove(...)
 */

#include "mini_max.h"
#include "transposition.h"
#include "bitops.h"
#include <stdint.h>

#ifndef HAS_CTZ64
// cppcheck-suppress preprocessorErrorDirective
#error "mini_max.c requires a count-trailing-zeros intrinsic (HAS_CTZ64 not defined). \
See src/MiniMax/bitops.h to add support for your compiler/platform."
#endif

/* Helper constants used by the evaluation and search. */
typedef enum
{
    AI_WIN_SCORE = 100,
    PLAYER_WIN_SCORE = -100,
    TIE_SCORE = 0,
    CONTINUE_SCORE = 1,
    INF = 101
} HelperScores;

/* Compile-time validation: terminal scores must fit in int16_t (transposition table storage) */
_Static_assert(AI_WIN_SCORE <= INT16_MAX && AI_WIN_SCORE >= INT16_MIN,
               "AI_WIN_SCORE must fit in int16_t");
_Static_assert(PLAYER_WIN_SCORE <= INT16_MAX && PLAYER_WIN_SCORE >= INT16_MIN,
               "PLAYER_WIN_SCORE must fit in int16_t");

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
 * Terminal evaluation using bitboard win detection:
 *  - +100 if a line completed by aiPlayer
 *  - -100 if a line completed by opponent
 *  -  0 for tie
 *  -  1 (CONTINUE_SCORE) if the game is not terminal
 */
static inline int boardScore(Bitboard board, char aiPlayer)
{
    /* Check if AI has won */
    uint64_t ai_pieces = (aiPlayer == 'x') ? board.x_pieces : board.o_pieces;
    if (bitboard_has_won(ai_pieces))
        return AI_WIN_SCORE;

    /* Check if opponent has won */
    uint64_t opponent_pieces = (aiPlayer == 'x') ? board.o_pieces : board.x_pieces;
    if (bitboard_has_won(opponent_pieces))
        return PLAYER_WIN_SCORE;

    /* Check if board is full (tie) */
    uint64_t occupied = board.x_pieces | board.o_pieces;
    if (occupied == VALID_POSITIONS_MASK)
        return TIE_SCORE;

    return CONTINUE_SCORE;
}

static int miniMaxLow(Bitboard board, char aiPlayer, int alpha, int beta, uint64_t hash);

/*
 * Maximizing ply (AI).
 * Returns best score achievable for aiPlayer from the current position.
 */
static int miniMaxHigh(Bitboard board, char aiPlayer, int alpha, int beta, uint64_t hash)
{
    /* Transposition table probe */
    int transposition_table_score;
    if (transposition_table_probe(hash, alpha, beta, &transposition_table_score))
    {
        return transposition_table_score;
    }

    int state = boardScore(board, aiPlayer);
    if (state != CONTINUE_SCORE)
    {
        /* terminal: cache and return raw score */
        transposition_table_store(hash, state, TRANSPOSITION_TABLE_EXACT);
        return state;
    }

    uint64_t empty = ~(board.x_pieces | board.o_pieces) & VALID_POSITIONS_MASK;
    int bestScore = -INF;
    int original_alpha = alpha;

    while (empty)
    {
        int bit = CTZ64(empty);
        empty &= empty - 1;
        int row = BIT_TO_ROW(bit);
        int col = BIT_TO_COL(bit);
        bitboard_make_move(&board, row, col, aiPlayer);
        uint64_t new_hash = zobrist_toggle(hash, row, col, aiPlayer);
        new_hash = zobrist_toggle_turn(new_hash); /* Toggle turn: AI → Opponent */
        int score = miniMaxLow(board, aiPlayer, alpha, beta, new_hash);
        bitboard_unmake_move(&board, row, col, aiPlayer);

        if (score > bestScore)
        {
            bestScore = score;
        }

        /* Early win return: stop searching if we found a winning move */
        if (bestScore == AI_WIN_SCORE)
            break;

        if (score > alpha)
            alpha = score;
        if (beta <= alpha)
            break; /* Beta cutoff */
    }

    /* Classify node type for transposition table storage */
    TranspositionTableNodeType store_type;
    if (bestScore >= beta)
    {
        store_type = TRANSPOSITION_TABLE_LOWERBOUND; /* Beta cutoff */
    }
    else if (bestScore <= original_alpha)
    {
        store_type = TRANSPOSITION_TABLE_UPPERBOUND; /* All moves were <= original alpha */
    }
    else
    {
        store_type = TRANSPOSITION_TABLE_EXACT; /* Exact score */
    }
    transposition_table_store(hash, bestScore, store_type);

    return bestScore;
}

/*
 * Minimizing ply (opponent).
 * Returns worst-case score for aiPlayer given optimal opponent play.
 */
static int miniMaxLow(Bitboard board, char aiPlayer, int alpha, int beta, uint64_t hash)
{
    /* Transposition table probe */
    int transposition_table_score;
    if (transposition_table_probe(hash, alpha, beta, &transposition_table_score))
    {
        return transposition_table_score;
    }

    int state = boardScore(board, aiPlayer);
    if (state != CONTINUE_SCORE)
    {
        /* terminal: cache and return raw score */
        transposition_table_store(hash, state, TRANSPOSITION_TABLE_EXACT);
        return state;
    }

    uint64_t empty = ~(board.x_pieces | board.o_pieces) & VALID_POSITIONS_MASK;
    int bestScore = INF;
    char opponent = (aiPlayer == 'x') ? 'o' : 'x';
    int original_beta = beta;

    while (empty)
    {
        int bit = CTZ64(empty);
        empty &= empty - 1;
        int row = BIT_TO_ROW(bit);
        int col = BIT_TO_COL(bit);
        bitboard_make_move(&board, row, col, opponent);
        uint64_t new_hash = zobrist_toggle(hash, row, col, opponent);
        new_hash = zobrist_toggle_turn(new_hash); /* Toggle turn: Opponent → AI */
        int score = miniMaxHigh(board, aiPlayer, alpha, beta, new_hash);
        bitboard_unmake_move(&board, row, col, opponent);

        if (score < bestScore)
        {
            bestScore = score;
        }

        /* Early win return: stop searching if opponent found a winning move */
        if (bestScore == PLAYER_WIN_SCORE)
            break;

        if (score < beta)
            beta = score;
        if (beta <= alpha)
            break; /* Alpha cutoff */
    }

    /* Classify node type for transposition table storage */
    TranspositionTableNodeType store_type;
    if (bestScore <= alpha)
    {
        store_type = TRANSPOSITION_TABLE_UPPERBOUND; /* Alpha cutoff */
    }
    else if (bestScore >= original_beta)
    {
        store_type = TRANSPOSITION_TABLE_LOWERBOUND; /* All moves were >= original beta */
    }
    else
    {
        store_type = TRANSPOSITION_TABLE_EXACT; /* Exact score */
    }
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

    int state = boardScore(board, aiPlayer);
    if (state != CONTINUE_SCORE)
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
    uint64_t hash = zobrist_hash(board, aiPlayer);

    while (empty)
    {
        int bit = CTZ64(empty);
        empty &= empty - 1;
        int row = BIT_TO_ROW(bit);
        int col = BIT_TO_COL(bit);
        bitboard_make_move(&board, row, col, aiPlayer);
        uint64_t new_hash = zobrist_toggle(hash, row, col, aiPlayer);
        new_hash = zobrist_toggle_turn(new_hash); /* Toggle turn: AI → Opponent */
        int score = miniMaxLow(board, aiPlayer, alpha, beta, new_hash);
        bitboard_unmake_move(&board, row, col, aiPlayer);

        if (score > bestScore)
        {
            bestScore = score;
            bestRow = row;
            bestCol = col;
            alpha = score;
        }

        /* Early win return: stop searching if we found a winning move */
        if (bestScore == AI_WIN_SCORE)
            break;
    }

    *out_row = bestRow;
    *out_col = bestCol;
}
