/*
 * Transposition Table Implementation
 * -----------------------------------
 * See transposition.h for API documentation.
 */

#include "transposition.h"
#include "bitops.h"
#include <stdlib.h>
#include <stdio.h>

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

/*
 * Zobrist key for side-to-move.
 * XORed into hash when it's the opponent's turn (miniMaxLow).
 */
static uint64_t zobrist_turn_key;

/* Transposition table */
static TranspositionTableEntry *transposition_table = NULL;
static size_t transposition_table_size = 0;
static size_t transposition_table_mask = 0; /* Bitmask for fast modulo (size - 1) */

/* SplitMix64 PRNG state for Zobrist key generation */
static uint64_t splitmix64_state = 0x9e3779b97f4a7c15ULL; /* Default seed (golden ratio) */

/*
 * SplitMix64: High-quality 64-bit PRNG
 */
static uint64_t splitmix64_next(void)
{
    uint64_t z = (splitmix64_state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

/*
 * Round up to next power of 2 for efficient modulo-free indexing.
 * Uses bit manipulation to avoid expensive modulo operation.
 */
static size_t round_up_power_of_2(size_t n)
{
    if (n == 0)
        return 1;

    /* Guard against overflow: if n > SIZE_MAX/2, can't safely round up */
    if (n > (SIZE_MAX >> 1))
        return (SIZE_MAX >> 1) + 1; /* Return largest power of 2 */

    /* Already a power of 2? */
    if ((n & (n - 1)) == 0)
        return n;

    /* Round up using bit manipulation */
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
#if SIZE_MAX > 0xFFFFFFFF
    n |= n >> 32; /* Only on 64-bit platforms */
#endif
    return n + 1;
}

/* Map player symbol to index for Zobrist key lookup */
static inline int player_to_index(char player)
{
    return (player == 'x') ? 0 : 1;
}

void zobrist_set_seed(uint64_t seed)
{
    splitmix64_state = seed;
}

void zobrist_init(void)
{
    /* Initialize piece keys using SplitMix64 */
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            for (int p = 0; p < 2; p++)
            {
                zobrist_keys[r][c][p] = splitmix64_next();
            }
        }
    }

    /* Initialize player perspective keys */
    zobrist_player_keys[0] = splitmix64_next();
    zobrist_player_keys[1] = splitmix64_next();

    /* Initialize side-to-move key */
    zobrist_turn_key = splitmix64_next();
}

uint64_t zobrist_hash(Bitboard board, char aiPlayer)
{
    /*
     * Hash encodes both position AND perspective (aiPlayer).
     * This is critical because the minimax score for a position depends on
     * who is maximizing. Without the aiPlayer key, we would return stale
     * scores from the wrong perspective when games alternate starting player.
     */
    uint64_t hash = zobrist_player_keys[player_to_index(aiPlayer)];

    /* Hash X pieces using bit scanning */
    uint64_t x_pieces = board.x_pieces;
    while (x_pieces)
    {
        int bit = CTZ64(x_pieces);
        int row = BIT_TO_ROW(bit);
        int col = BIT_TO_COL(bit);
        hash ^= zobrist_keys[row][col][0];
        x_pieces &= x_pieces - 1;
    }

    /* Hash O pieces */
    uint64_t o_pieces = board.o_pieces;
    while (o_pieces)
    {
        int bit = CTZ64(o_pieces);
        int row = BIT_TO_ROW(bit);
        int col = BIT_TO_COL(bit);
        hash ^= zobrist_keys[row][col][1];
        o_pieces &= o_pieces - 1;
    }

    return hash;
}

uint64_t zobrist_toggle(uint64_t hash, int row, int col, char player)
{
    return hash ^ zobrist_keys[row][col][player_to_index(player)];
}

uint64_t zobrist_toggle_turn(uint64_t hash)
{
    return hash ^ zobrist_turn_key;
}

void transposition_table_init(size_t size)
{
    /* Free existing table if reinitializing */
    if (transposition_table != NULL)
    {
        free(transposition_table);
        transposition_table = NULL;
    }

    /* Handle size 0: disable TT entirely */
    if (size == 0)
    {
        transposition_table = NULL;
        transposition_table_size = 0;
        transposition_table_mask = 0;
        return;
    }

    /* Round up to power of 2 for efficient indexing */
    transposition_table_size = round_up_power_of_2(size);
    transposition_table_mask = transposition_table_size - 1;

    transposition_table = (TranspositionTableEntry *)calloc(transposition_table_size, sizeof(TranspositionTableEntry));

    if (transposition_table == NULL)
    {
        double table_size_mb = ((double)transposition_table_size * (double)sizeof(TranspositionTableEntry)) / (1024.0 * 1024.0);
        fprintf(stderr, "Warning: Failed to allocate transposition table (%zu entries requested, %zu actual, %.1f MB)\n",
                size, transposition_table_size,
                table_size_mb);
        fprintf(stderr, "Continuing without transposition table.\n");
        transposition_table_size = 0;
        transposition_table_mask = 0;
    }
}

void transposition_table_free(void)
{
    if (transposition_table != NULL)
    {
        free(transposition_table);
        transposition_table = NULL;
    }
    transposition_table_size = 0;
    transposition_table_mask = 0;
}

int transposition_table_probe(uint64_t hash, int alpha, int beta,
                              int *restrict out_score)
{
    if (transposition_table == NULL || transposition_table_size == 0)
    {
        return 0;
    }

    size_t index = hash & transposition_table_mask;
    const TranspositionTableEntry *entry = &transposition_table[index];

    /* Empty slot */
    if (entry->occupied == 0)
    {
        return 0;
    }

    /* Hash collision */
    if (entry->hash != hash)
    {
        return 0;
    }

    /* No depth check needed - scores are now depth-independent */

    int score = entry->score;

    /* Use stored score based on node type and bounds */
    if (entry->type == TRANSPOSITION_TABLE_EXACT ||
        (entry->type == TRANSPOSITION_TABLE_LOWERBOUND && score >= beta) ||
        (entry->type == TRANSPOSITION_TABLE_UPPERBOUND && score <= alpha))
    {
        *out_score = score;
        return 1;
    }

    return 0;
}

void transposition_table_store(uint64_t hash, int score, TranspositionTableNodeType type)
{
    if (transposition_table == NULL || transposition_table_size == 0)
    {
        return;
    }

    size_t index = hash & transposition_table_mask;
    TranspositionTableEntry *entry = &transposition_table[index];

    /* Replacement strategy: always replace */
    entry->hash = hash;
    entry->score = (int16_t)score;
    entry->type = (uint8_t)type;
    entry->occupied = 1;
}
