/*
 * Minimax search with alpha–beta pruning for Tic-Tac-Toe
 * -------------------------------------------------------
 *
 * This file implements a deterministic Minimax engine with:
 *  - Alpha–beta pruning
 *  - Lightweight move ordering (center > diagonals/adjacent > others)
 *  - Early cutoffs via last-move win checks and last-move tie shortcut
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

/* Collect all empty cells in row-major order. */
static void findEmptySpots(const char board[BOARD_SIZE][BOARD_SIZE], MoveList *out_emptySpots)
{
    out_emptySpots->count = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (board[i][j] == ' ')
            {
                out_emptySpots->moves[out_emptySpots->count++] = (Move){i, j};
            }
        }
    }
}

/*
 * Heuristic weight for move ordering:
 *  - 4: exact center (Manhattan distance 0 to center)
 *  - 3: on a diagonal OR Manhattan distance 1 from center
 *  - 2: everything else
 * Works for odd and even BOARD_SIZE by using two central indices.
 */
static int moveWeight(int row, int col)
{
    int lowerMiddle = (BOARD_SIZE - 1) / 2;
    int upperMiddle = BOARD_SIZE / 2;

    int distanceRowToLowerMiddle = row - lowerMiddle;
    if (distanceRowToLowerMiddle < 0)
        distanceRowToLowerMiddle = -distanceRowToLowerMiddle;

    int distanceRowToUpperMiddle = row - upperMiddle;
    if (distanceRowToUpperMiddle < 0)
        distanceRowToUpperMiddle = -distanceRowToUpperMiddle;

    int minimalRowDistance = distanceRowToLowerMiddle < distanceRowToUpperMiddle ? distanceRowToLowerMiddle : distanceRowToUpperMiddle;

    int distanceColumnToLowerMiddle = col - lowerMiddle;
    if (distanceColumnToLowerMiddle < 0)
        distanceColumnToLowerMiddle = -distanceColumnToLowerMiddle;

    int distanceColumnToUpperMiddle = col - upperMiddle;
    if (distanceColumnToUpperMiddle < 0)
        distanceColumnToUpperMiddle = -distanceColumnToUpperMiddle;

    int minimalColumnDistance = distanceColumnToLowerMiddle < distanceColumnToUpperMiddle ? distanceColumnToLowerMiddle : distanceColumnToUpperMiddle;

    int manhattanDistanceToCenter = minimalRowDistance + minimalColumnDistance;

    if (manhattanDistanceToCenter == 0)
        return 4;

    if (row == col || row + col == BOARD_SIZE - 1)
        return 3;

    if (manhattanDistanceToCenter == 1)
        return 3;

    return 2;
}

/*
 * Fast win check based on the last move applied.
 * Only scans the affected row, column, and relevant diagonal(s).
 */
static int didLastMoveWin(const char board[BOARD_SIZE][BOARD_SIZE], int row, int col)
{
    char player = board[row][col];
    if (player == ' ')
        return 0;

    int win = 1;
    for (int c = 0; c < BOARD_SIZE; ++c)
    {
        if (board[row][c] != player)
        {
            win = 0;
            break;
        }
    }
    if (win)
        return 1;

    win = 1;
    for (int r = 0; r < BOARD_SIZE; ++r)
    {
        if (board[r][col] != player)
        {
            win = 0;
            break;
        }
    }
    if (win)
        return 1;

    if (row == col)
    {
        win = 1;
        for (int i = 0; i < BOARD_SIZE; ++i)
        {
            if (board[i][i] != player)
            {
                win = 0;
                break;
            }
        }
        if (win)
            return 1;
    }

    if (row + col == BOARD_SIZE - 1)
    {
        win = 1;
        for (int i = 0; i < BOARD_SIZE; ++i)
        {
            if (board[i][BOARD_SIZE - 1 - i] != player)
            {
                win = 0;
                break;
            }
        }
        if (win)
            return 1;
    }

    return 0;
}

/*
 * Partition moves by weight into three buckets and concatenate them
 * (4 -> 3 -> 2). Within each bucket, original row-major ordering is kept,
 * ensuring deterministic results for tied scores.
 */
static void orderMoves(MoveList *moves)
{
    Move movesWithWeightFour[MAX_MOVES];
    Move movesWithWeightThree[MAX_MOVES];
    Move movesWithWeightTwo[MAX_MOVES];
    int movesWithWeightFourCount = 0;
    int movesWithWeightThreeCount = 0;
    int movesWithWeightTwoCount = 0;

    for (int moveIndex = 0; moveIndex < moves->count; ++moveIndex)
    {
        Move move = moves->moves[moveIndex];
        int weight = moveWeight(move.row, move.col);
        if (weight == 4)
        {
            movesWithWeightFour[movesWithWeightFourCount++] = move;
        }
        else if (weight == 3)
        {
            movesWithWeightThree[movesWithWeightThreeCount++] = move;
        }
        else
        {
            movesWithWeightTwo[movesWithWeightTwoCount++] = move;
        }
    }

    int writeIndex = 0;
    for (int index = 0; index < movesWithWeightFourCount; ++index)
    {
        moves->moves[writeIndex++] = movesWithWeightFour[index];
    }
    for (int index = 0; index < movesWithWeightThreeCount; ++index)
    {
        moves->moves[writeIndex++] = movesWithWeightThree[index];
    }
    for (int index = 0; index < movesWithWeightTwoCount; ++index)
    {
        moves->moves[writeIndex++] = movesWithWeightTwo[index];
    }
}

/*
 * Terminal evaluation:
 *  - +100 if a line completed by aiPlayer
 *  - -100 if a line completed by opponent
 *  -  0 for tie
 *  -  1 (CONTINUE_SCORE) if the game is not terminal
 */
static int boardScore(const char board[BOARD_SIZE][BOARD_SIZE], char aiPlayer)
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        char first = board[i][0];

        if (first == ' ')
            continue;

        int allSame = 1;

        for (int j = 1; j < BOARD_SIZE; j++)
        {
            if (board[i][j] != first)
            {
                allSame = 0;
                break;
            }
        }

        if (allSame)
            return (first == aiPlayer) ? AI_WIN_SCORE : PLAYER_WIN_SCORE;
    }

    for (int j = 0; j < BOARD_SIZE; j++)
    {
        char first = board[0][j];

        if (first == ' ')
            continue;

        int allSame = 1;

        for (int i = 1; i < BOARD_SIZE; i++)
        {
            if (board[i][j] != first)
            {
                allSame = 0;
                break;
            }
        }

        if (allSame)
            return (first == aiPlayer) ? AI_WIN_SCORE : PLAYER_WIN_SCORE;
    }

    char first = board[0][0];

    if (first != ' ')
    {
        int allSame = 1;

        for (int i = 1; i < BOARD_SIZE; i++)
        {
            if (board[i][i] != first)
            {
                allSame = 0;
                break;
            }
        }

        if (allSame)
            return (first == aiPlayer) ? AI_WIN_SCORE : PLAYER_WIN_SCORE;
    }

    first = board[0][BOARD_SIZE - 1];

    if (first != ' ')
    {
        int allSame = 1;

        for (int i = 1; i < BOARD_SIZE; i++)
        {
            if (board[i][BOARD_SIZE - 1 - i] != first)
            {
                allSame = 0;
                break;
            }
        }

        if (allSame)
            return (first == aiPlayer) ? AI_WIN_SCORE : PLAYER_WIN_SCORE;
    }

    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            if (board[i][j] == ' ')
                return CONTINUE_SCORE;
        }
    }

    return TIE_SCORE;
}

static int miniMaxLow(char board[BOARD_SIZE][BOARD_SIZE], char aiPlayer, int depth, int alpha, int beta, uint64_t hash);

/*
 * Maximizing ply (AI).
 * Returns best score achievable for aiPlayer from the current position.
 */
static int miniMaxHigh(char board[BOARD_SIZE][BOARD_SIZE], char aiPlayer, int depth, int alpha, int beta, uint64_t hash)
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
    orderMoves(&emptySpots);
    int bestScore = -INF;
    Move bestMove = {-1, -1};
    int original_alpha = alpha;

    for (int i = 0; i < emptySpots.count; i++)
    {
        Move move = emptySpots.moves[i];
        board[move.row][move.col] = aiPlayer;
        uint64_t new_hash = zobrist_toggle(hash, move.row, move.col, aiPlayer);
        int score;
        if (didLastMoveWin(board, move.row, move.col))
        {
            /* immediate win after this move; prefer faster wins */
            score = AI_WIN_SCORE - (depth + 1);
        }
        else if (emptySpots.count == 1)
        {
            score = TIE_SCORE;
        }
        else
        {
            score = miniMaxLow(board, aiPlayer, depth + 1, alpha, beta, new_hash);
        }
        board[move.row][move.col] = ' ';

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = move;
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
    transposition_table_store(hash, depth, bestScore, store_type, bestMove.row, bestMove.col);

    return bestScore;
}

/*
 * Minimizing ply (opponent).
 * Returns worst-case score for aiPlayer given optimal opponent play.
 */
static int miniMaxLow(char board[BOARD_SIZE][BOARD_SIZE], char aiPlayer, int depth, int alpha, int beta, uint64_t hash)
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
    orderMoves(&emptySpots);
    int bestScore = INF;
    Move bestMove = {-1, -1};
    char opponent = (aiPlayer == 'x') ? 'o' : 'x';
    int original_beta = beta;

    for (int i = 0; i < emptySpots.count; i++)
    {
        Move move = emptySpots.moves[i];
        board[move.row][move.col] = opponent;
        uint64_t new_hash = zobrist_toggle(hash, move.row, move.col, opponent);
        int score;
        if (didLastMoveWin(board, move.row, move.col))
        {
            /* opponent just won; later losses are (slightly) better */
            score = PLAYER_WIN_SCORE + (depth + 1);
        }
        else if (emptySpots.count == 1)
        {
            score = TIE_SCORE;
        }
        else
        {
            score = miniMaxHigh(board, aiPlayer, depth + 1, alpha, beta, new_hash);
        }
        board[move.row][move.col] = ' ';

        if (score < bestScore)
        {
            bestScore = score;
            bestMove = move;
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
    transposition_table_store(hash, depth, bestScore, store_type, bestMove.row, bestMove.col);

    return bestScore;
}

/*
 * Public entry: select the best move for aiPlayer.
 * Short-circuits:
 *  - Terminal board -> (-1, -1)
 *  - Empty board    -> center (BOARD_SIZE/2, BOARD_SIZE/2) without searching
 */
void getAiMove(char board[BOARD_SIZE][BOARD_SIZE], char aiPlayer, int *out_row, int *out_col)
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

    orderMoves(&emptySpots);

    int alpha = -INF;
    int beta = INF;
    Move bestMove = emptySpots.moves[0];
    int bestScore = -INF;
    uint64_t hash = zobrist_hash(board, aiPlayer);

    for (int i = 0; i < emptySpots.count; ++i)
    {
        Move move = emptySpots.moves[i];
        board[move.row][move.col] = aiPlayer;

        if (didLastMoveWin(board, move.row, move.col))
        {
            board[move.row][move.col] = ' ';
            *out_row = move.row;
            *out_col = move.col;
            return;
        }

        uint64_t new_hash = zobrist_toggle(hash, move.row, move.col, aiPlayer);
        int score = miniMaxLow(board, aiPlayer, 1, alpha, beta, new_hash);
        board[move.row][move.col] = ' ';

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
