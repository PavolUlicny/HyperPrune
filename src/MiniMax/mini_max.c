/*
 * Minimax search with alpha–beta pruning for Tic-Tac-Toe
 * -------------------------------------------------------
 *
 * This file implements a deterministic Minimax engine with:
 *  - Alpha–beta pruning
 *  - Depth-adjusted terminal scoring (prefer faster wins, delay losses)
 *  - Simple opening heuristic: play center on empty board
 *  - Transposition table with Zobrist hashing for position caching
 *
 * Public entry point: getAiMove(...)
 */

#include "mini_max.h"
#include "transposition.h"
#include <limits.h>

/* A single board coordinate (row, col). */
typedef struct
{
    int row;
    int col;
} Move;

/* A trivial fixed-size container for generated legal moves. */
typedef struct
{
    Move moves[MAX_MOVES];
    int count;
} MoveList;

/* Helper constants used by the evaluation and search. */
typedef enum
{
    AI_WIN_SCORE = 100,
    PLAYER_WIN_SCORE = -100,
    TIE_SCORE = 0,
    CONTINUE_SCORE = 1,
    INF = INT_MAX
} HelperScores;

/*
 * Safe mask for valid board positions.
 * Avoids undefined behavior when MAX_MOVES == 64 (1ULL << 64 is UB).
 */
static inline uint64_t valid_positions_mask(void)
{
    return (MAX_MOVES == 64) ? ~0ULL : ((1ULL << MAX_MOVES) - 1);
}

/* Collect all empty cells using bit scanning. */
static void findEmptySpots(Bitboard board, MoveList *out_emptySpots)
{
    out_emptySpots->count = 0;

    uint64_t empty = ~(board.x_pieces | board.o_pieces);
    empty &= valid_positions_mask(); /* Mask valid positions */

#ifdef __GNUC__
    /* Use compiler builtin for fast bit scanning */
    while (empty)
    {
        int bit = __builtin_ctzll(empty); /* Count trailing zeros (O(1)) */
        out_emptySpots->moves[out_emptySpots->count++] = (Move){
            .row = BIT_TO_ROW(bit),
            .col = BIT_TO_COL(bit)};
        empty &= empty - 1; /* Clear least significant bit */
    }
#else
    /* Fallback for non-GCC compilers */
    for (int i = 0; i < MAX_MOVES; i++)
    {
        if (empty & (1ULL << i))
        {
            out_emptySpots->moves[out_emptySpots->count++] = (Move){
                .row = BIT_TO_ROW(i),
                .col = BIT_TO_COL(i)};
        }
    }
#endif
}

/*
 * Terminal evaluation using bitboard win detection:
 *  - +100 if a line completed by aiPlayer
 *  - -100 if a line completed by opponent
 *  -  0 for tie
 *  -  1 (CONTINUE_SCORE) if the game is not terminal
 */
static int boardScore(Bitboard board, char aiPlayer)
{
    /* Check if AI has won */
    uint64_t ai_pieces = (aiPlayer == 'x') ? board.x_pieces : board.o_pieces;
    if (bitboard_has_won(ai_pieces))
        return AI_WIN_SCORE;

    /* Check if opponent has won */
    char opponent = (aiPlayer == 'x') ? 'o' : 'x';
    uint64_t opponent_pieces = (opponent == 'x') ? board.x_pieces : board.o_pieces;
    if (bitboard_has_won(opponent_pieces))
        return PLAYER_WIN_SCORE;

    /* Check if board is full (tie) */
    uint64_t occupied = board.x_pieces | board.o_pieces;
    if (occupied == valid_positions_mask())
        return TIE_SCORE;

    return CONTINUE_SCORE;
}

static int miniMaxLow(Bitboard board, char aiPlayer, int depth, int alpha, int beta, uint64_t hash);

/*
 * Maximizing ply (AI).
 * Returns best score achievable for aiPlayer from the current position.
 */
static int miniMaxHigh(Bitboard board, char aiPlayer, int depth, int alpha, int beta, uint64_t hash)
{
    /* Transposition table probe */
    int transposition_table_score;
    TranspositionTableNodeType transposition_table_type;
    if (transposition_table_probe(hash, depth, alpha, beta, &transposition_table_score, &transposition_table_type))
    {
        return transposition_table_score;
    }

    int state = boardScore(board, aiPlayer);
    if (state != CONTINUE_SCORE)
    {
        /* terminal: propagate depth-adjusted values */
        if (state == TIE_SCORE)
            return TIE_SCORE;

        if (state > 0)
            return state - depth;

        return state + depth;
    }

    MoveList emptySpots;
    findEmptySpots(board, &emptySpots);
    int bestScore = -INF;
    int original_alpha = alpha;

    for (int i = 0; i < emptySpots.count; i++)
    {
        Move move = emptySpots.moves[i];
        bitboard_make_move(&board, move.row, move.col, aiPlayer);
        uint64_t new_hash = zobrist_toggle(hash, move.row, move.col, aiPlayer);
        int score = miniMaxLow(board, aiPlayer, depth + 1, alpha, beta, new_hash);
        bitboard_unmake_move(&board, move.row, move.col, aiPlayer);

        if (score > bestScore)
        {
            bestScore = score;
        }

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
        store_type = TRANSPOSITION_TABLE_EXACT; /* PV node */
    }
    transposition_table_store(hash, depth, bestScore, store_type);

    return bestScore;
}

/*
 * Minimizing ply (opponent).
 * Returns worst-case score for aiPlayer given optimal opponent play.
 */
static int miniMaxLow(Bitboard board, char aiPlayer, int depth, int alpha, int beta, uint64_t hash)
{
    /* Transposition table probe */
    int transposition_table_score;
    TranspositionTableNodeType transposition_table_type;
    if (transposition_table_probe(hash, depth, alpha, beta, &transposition_table_score, &transposition_table_type))
    {
        return transposition_table_score;
    }

    int state = boardScore(board, aiPlayer);
    if (state != CONTINUE_SCORE)
    {
        /* terminal: propagate depth-adjusted values */
        if (state == TIE_SCORE)
            return TIE_SCORE;

        if (state > 0)
            return state - depth;

        return state + depth;
    }

    MoveList emptySpots;
    findEmptySpots(board, &emptySpots);
    int bestScore = INF;
    char opponent = (aiPlayer == 'x') ? 'o' : 'x';
    int original_beta = beta;

    for (int i = 0; i < emptySpots.count; i++)
    {
        Move move = emptySpots.moves[i];
        bitboard_make_move(&board, move.row, move.col, opponent);
        uint64_t new_hash = zobrist_toggle(hash, move.row, move.col, opponent);
        int score = miniMaxHigh(board, aiPlayer, depth + 1, alpha, beta, new_hash);
        bitboard_unmake_move(&board, move.row, move.col, opponent);

        if (score < bestScore)
        {
            bestScore = score;
        }

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
        store_type = TRANSPOSITION_TABLE_EXACT; /* PV node */
    }
    transposition_table_store(hash, depth, bestScore, store_type);

    return bestScore;
}

/*
 * Public entry: select the best move for aiPlayer.
 * Short-circuits:
 *  - Terminal board -> (-1, -1)
 *  - Empty board    -> center (BOARD_SIZE/2, BOARD_SIZE/2) without searching
 */
void getAiMove(Bitboard board, char aiPlayer, int *out_row, int *out_col)
{
    int state = boardScore(board, aiPlayer);
    if (state != CONTINUE_SCORE)
    {
        *out_row = -1;
        *out_col = -1;
        return;
    }

    MoveList emptySpots;
    findEmptySpots(board, &emptySpots);

    if (emptySpots.count == MAX_MOVES)
    {
        /* center square; for even boards, lower-right of the central 2×2 */
        *out_row = BOARD_SIZE / 2;
        *out_col = BOARD_SIZE / 2;
        return;
    }

    if (emptySpots.count == 1)
    {
        *out_row = emptySpots.moves[0].row;
        *out_col = emptySpots.moves[0].col;
        return;
    }

    int alpha = -INF;
    int beta = INF;
    Move bestMove = {-1, -1};
    int bestScore = -INF;
    uint64_t hash = zobrist_hash(board, aiPlayer);

    for (int i = 0; i < emptySpots.count; ++i)
    {
        Move move = emptySpots.moves[i];
        bitboard_make_move(&board, move.row, move.col, aiPlayer);
        uint64_t new_hash = zobrist_toggle(hash, move.row, move.col, aiPlayer);
        int score = miniMaxLow(board, aiPlayer, 1, alpha, beta, new_hash);
        bitboard_unmake_move(&board, move.row, move.col, aiPlayer);

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = move;
            alpha = score;
        }
    }

    *out_row = bestMove.row;
    *out_col = bestMove.col;
}
