/*
 * Transposition Table Implementation
 * -----------------------------------
 * See transposition.h for API documentation.
 */

#include "transposition.h"
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

/*
 * Zobrist keys: [row][col][player_index]
 * player_index: 0='x', 1='o'
 */
static uint64_t zobrist_keys[BOARD_SIZE][BOARD_SIZE][2];

/*
 * Zobrist keys for aiPlayer perspective.
 * XORed into hash to distinguish positions with different maximizing players.
 */
static uint64_t zobrist_player_keys[2];

/* Transposition table and statistics */
static TranspositionTableEntry *transposition_table = NULL;
static size_t transposition_table_size = 0;
static size_t transposition_table_hits = 0;
static size_t transposition_table_misses = 0;
static size_t transposition_table_collisions = 0;

/* Map player symbol to index for Zobrist key lookup */
static inline int player_to_index(char player)
{
    return (player == 'x') ? 0 : 1;
}

void zobrist_init(void)
{
    srand((unsigned int)time(NULL));

    /* Initialize piece keys */
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            for (int p = 0; p < 2; p++)
            {
                zobrist_keys[r][c][p] = ((uint64_t)rand() << 32) | (uint64_t)rand();
            }
        }
    }

    /* Initialize player perspective keys */
    zobrist_player_keys[0] = ((uint64_t)rand() << 32) | (uint64_t)rand();
    zobrist_player_keys[1] = ((uint64_t)rand() << 32) | (uint64_t)rand();
}

uint64_t zobrist_hash(char board[BOARD_SIZE][BOARD_SIZE], char aiPlayer)
{
    /*
     * Hash encodes both position AND perspective (aiPlayer).
     * This is critical because the minimax score for a position depends on
     * who is maximizing. Without the aiPlayer key, we would return stale
     * scores from the wrong perspective when games alternate starting player.
     */
    uint64_t hash = zobrist_player_keys[player_to_index(aiPlayer)];

    for (int r = 0; r < BOARD_SIZE; r++)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            if (board[r][c] != ' ')
            {
                hash ^= zobrist_keys[r][c][player_to_index(board[r][c])];
            }
        }
    }

    return hash;
}

uint64_t zobrist_toggle(uint64_t hash, int row, int col, char player)
{
    return hash ^ zobrist_keys[row][col][player_to_index(player)];
}

void transposition_table_init(size_t size)
{
    transposition_table_size = size;
    transposition_table = (TranspositionTableEntry *)calloc(size, sizeof(TranspositionTableEntry));

    if (transposition_table == NULL)
    {
        fprintf(stderr, "Warning: Failed to allocate transposition table (%zu entries, %.1f MB)\n",
                size, (size * sizeof(TranspositionTableEntry)) / (1024.0 * 1024.0));
        fprintf(stderr, "Continuing without transposition table - performance will be degraded.\n");
        transposition_table_size = 0;
    }

    transposition_table_hits = 0;
    transposition_table_misses = 0;
    transposition_table_collisions = 0;
}

void transposition_table_free(void)
{
    if (transposition_table != NULL)
    {
        free(transposition_table);
        transposition_table = NULL;
    }
    transposition_table_size = 0;
}

int transposition_table_probe(uint64_t hash, int depth, int alpha, int beta,
                               int *out_score, TranspositionTableNodeType *out_type)
{
    if (transposition_table == NULL || transposition_table_size == 0)
    {
        return 0;
    }

    size_t index = hash % transposition_table_size;
    TranspositionTableEntry *entry = &transposition_table[index];

    /* Empty slot */
    if (entry->hash == 0)
    {
        transposition_table_misses++;
        return 0;
    }

    /* Hash collision */
    if (entry->hash != hash)
    {
        transposition_table_collisions++;
        return 0;
    }

    /* Depth-based cutoff: only use if stored depth >= current depth */
    if (entry->depth < depth)
    {
        transposition_table_misses++;
        return 0;
    }

    int score = entry->score;

    /* Use stored score based on node type and bounds */
    if (entry->type == TRANSPOSITION_TABLE_EXACT)
    {
        *out_score = score;
        *out_type = TRANSPOSITION_TABLE_EXACT;
        transposition_table_hits++;
        return 1;
    }
    else if (entry->type == TRANSPOSITION_TABLE_LOWERBOUND && score >= beta)
    {
        /* We stored a lower bound that's >= beta, so we can cut off */
        *out_score = score;
        *out_type = TRANSPOSITION_TABLE_LOWERBOUND;
        transposition_table_hits++;
        return 1;
    }
    else if (entry->type == TRANSPOSITION_TABLE_UPPERBOUND && score <= alpha)
    {
        /* We stored an upper bound that's <= alpha, so we can cut off */
        *out_score = score;
        *out_type = TRANSPOSITION_TABLE_UPPERBOUND;
        transposition_table_hits++;
        return 1;
    }

    transposition_table_misses++;
    return 0;
}

void transposition_table_store(uint64_t hash, int depth, int score, TranspositionTableNodeType type,
                                int best_row, int best_col)
{
    if (transposition_table == NULL || transposition_table_size == 0)
    {
        return;
    }

    size_t index = hash % transposition_table_size;
    TranspositionTableEntry *entry = &transposition_table[index];

    /* Replacement strategy: always replace */
    entry->hash = hash;
    entry->score = (int16_t)score;
    entry->depth = (uint16_t)depth;
    entry->type = (uint8_t)type;
    entry->best_row = (int8_t)best_row;
    entry->best_col = (int8_t)best_col;
    entry->padding = 0;
}

void transposition_table_get_stats(size_t *hits, size_t *misses, size_t *collisions)
{
    if (hits != NULL)
        *hits = transposition_table_hits;
    if (misses != NULL)
        *misses = transposition_table_misses;
    if (collisions != NULL)
        *collisions = transposition_table_collisions;
}
