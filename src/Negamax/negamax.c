/*
 * Negamax search with alpha–beta pruning for Tic-Tac-Toe
 * -------------------------------------------------------
 *
 * This file implements a deterministic Negamax engine with:
 *  - Alpha–beta pruning
 *  - Terminal-only scoring (win/loss/tie evaluation)
 *  - Simple opening heuristic: play center on empty board
 *  - Transposition table with Zobrist hashing for position caching
 *  - Killer move heuristic: try refutation moves first at each depth
 *  - History heuristic: order remaining moves by cumulative beta-cutoff count
 *
 * Negamax formulation: every node is always from the current player's
 * perspective. The parent receives -negamax(child, -beta, -alpha), so scores
 * are relative to the player-to-move rather than a fixed "AI player".
 *
 * Public entry point: getAiMove(...)
 */

#include "negamax.h"
#include "transposition.h"
#include "bitops.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

/* Helper constants used by the evaluation and search. */
typedef enum
{
    WIN = 100, /* Current player wins */
    TIE = 0,
    INF = 101,         /* Alpha-beta window boundary (never stored in TT) */
    NOT_TERMINAL = 102 /* boardScore() sentinel: game still in progress (never stored in TT).
                        * Exceeds WIN to stay outside the storable score range [-WIN, WIN]. */
} HelperScores;

/* Compile-time validation: scores must fit in int16_t (transposition table storage).
 * INF/-INF are never stored, but asserting them guards against future constant changes.
 * NOT_TERMINAL is also never stored; assert it stays outside [-WIN, WIN]. */
_Static_assert(WIN <= INT16_MAX && (-WIN) >= INT16_MIN &&
                   INF <= INT16_MAX && (-INF) >= INT16_MIN,
               "WIN, -WIN, INF, and -INF must fit in int16_t");
_Static_assert(NOT_TERMINAL > WIN,
               "NOT_TERMINAL must exceed WIN to stay outside the storable score range");

/*
 * Killer moves and history heuristic tables.
 * Cleared at the start of each getAiMove() call; not shared across calls.
 *
 * killers[depth][0..1]: bit indices of moves that caused cutoffs (beta or WIN)
 *   at this depth. -1 = unused. Tried before other moves to get early cutoffs.
 * history[bit]: cumulative beta-cutoff count for each move across the whole search.
 *   Higher = more likely to cause a cutoff. Used to order non-killer moves.
 */
static int killers[MAX_MOVES][2];
static int history[MAX_MOVES];

static inline void killers_update(int depth, int bit)
{
    assert(depth < MAX_MOVES); /* max reachable depth is MAX_MOVES-3; array sized MAX_MOVES */
    if (killers[depth][0] != bit)
    {
        killers[depth][1] = killers[depth][0];
        killers[depth][0] = bit;
    }
}

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
 *   TIE (0)  : board is full → draw
 *   NOT_TERMINAL   : game not yet terminal
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
        return TIE;

    return NOT_TERMINAL;
}

/*
 * Negamax search with alpha-beta pruning.
 * Returns the best score achievable for currentPlayer from the current position.
 * All scores are relative to currentPlayer (positive = good for currentPlayer).
 */
static int negamax(Bitboard board, char currentPlayer, int alpha, int beta, uint64_t hash,
                   int depth)
{
    /* Transposition table probe */
    int tt_score;
    if (transposition_table_probe(hash, beta, &tt_score))
        return tt_score;

    /* Terminal check: did the previous player win? */
    int state = boardScore(board, currentPlayer);
    if (state != NOT_TERMINAL)
    {
        /* Terminal: cache and return exact score */
        transposition_table_store(hash, state, TRANSPOSITION_TABLE_EXACT);
        return state;
    }

    char opponent = (currentPlayer == 'x') ? 'o' : 'x';
    uint64_t empty = ~(board.x_pieces | board.o_pieces) & VALID_POSITIONS_MASK;
    int bestScore = -INF;
    uint64_t tried = 0; /* bitmask of moves already tried via killer slots */

    /* Phase 1: try killer moves first */
    for (int k = 0; k < 2; k++)
    {
        int bit = killers[depth][k];
        if (bit < 0 || !((empty >> bit) & 1))
            continue;
        tried |= 1ULL << bit;
        bitboard_make_move(&board, bit, currentPlayer);
        uint64_t new_hash = zobrist_toggle(hash, bit, currentPlayer);
        new_hash = zobrist_toggle_turn(new_hash);
        int score = -negamax(board, opponent, -beta, -alpha, new_hash, depth + 1);
        bitboard_unmake_move(&board, bit, currentPlayer);
        if (score > bestScore)
            bestScore = score;
        if (bestScore == WIN)
        {
            killers_update(depth, bit);
            /* history[bit] intentionally not updated: killers are depth-local
             * heuristics, and giving a killer's WIN result global history credit
             * inflates Phase 2 ordering at all depths in ways that interact with
             * the shared TT across getAiMove calls and break correctness. */
            goto move_loop_done;
        }
        if (score > alpha)
            alpha = score;
        if (beta <= alpha)
        {
            killers_update(depth, bit);
            history[bit]++;
            goto move_loop_done;
        }
    }

    /* Phase 2: remaining moves in descending history order */
    {
        uint64_t rem = empty & ~tried;
        while (rem)
        {
            /* Linear scan for the move with the highest history score */
            int best_bit = CTZ64(rem);
            int best_hist = history[best_bit];
            uint64_t scan = rem & (rem - 1);
            while (scan)
            {
                int b = CTZ64(scan);
                if (history[b] > best_hist)
                {
                    best_hist = history[b];
                    best_bit = b;
                }
                scan &= scan - 1;
            }
            rem &= ~(1ULL << best_bit);
            bitboard_make_move(&board, best_bit, currentPlayer);
            uint64_t new_hash = zobrist_toggle(hash, best_bit, currentPlayer);
            new_hash = zobrist_toggle_turn(new_hash);
            int score = -negamax(board, opponent, -beta, -alpha, new_hash, depth + 1);
            bitboard_unmake_move(&board, best_bit, currentPlayer);
            if (score > bestScore)
                bestScore = score;
            if (bestScore == WIN)
            {
                killers_update(depth, best_bit);
                goto move_loop_done;
            }
            if (score > alpha)
                alpha = score;
            if (beta <= alpha)
            {
                killers_update(depth, best_bit);
                history[best_bit]++;
                goto move_loop_done;
            }
        }
    }
move_loop_done:;

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
    int bestBit = -1;
    int bestScore = -INF;
    uint64_t hash = zobrist_hash(board);
    if (aiPlayer == 'o')
        hash = zobrist_toggle_turn(hash); /* O to move: root hash must include turn key */
    char opponent = (aiPlayer == 'x') ? 'o' : 'x';

    /* Clear killer and history tables for this search */
    memset(killers, -1, sizeof(killers));
    memset(history, 0, sizeof(history));

    while (empty)
    {
        int bit = CTZ64(empty);
        empty &= empty - 1;
        bitboard_make_move(&board, bit, aiPlayer);
        uint64_t new_hash = zobrist_toggle(hash, bit, aiPlayer);
        new_hash = zobrist_toggle_turn(new_hash);
        int score = -negamax(board, opponent, -beta, -alpha, new_hash, 0);
        bitboard_unmake_move(&board, bit, aiPlayer);

        if (score > bestScore)
        {
            bestScore = score;
            bestBit = bit;
            alpha = score;
        }

        /* Early exit: found a winning move */
        if (bestScore == WIN)
            break;
    }

    *out_row = BIT_TO_ROW(bestBit);
    *out_col = BIT_TO_COL(bestBit);
}
