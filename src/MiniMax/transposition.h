/*
 * Transposition Table with Zobrist Hashing
 * -----------------------------------------
 * Provides a hash-based cache for Minimax search results to avoid
 * re-evaluating identical positions reached via different move orders.
 *
 * Key components:
 *  - Zobrist hashing: fast incremental position hashing via XOR
 *  - Transposition table storage: hash table mapping positions to (score, depth, bounds)
 *  - Replacement strategy: always-replace for hash collisions
 *
 * Usage:
 *  1. Call zobrist_init() once at program startup
 *  2. Call transposition_table_init(size) to allocate the transposition table
 *  3. During search, use zobrist_hash/zobrist_toggle for position keys
 *  4. Call transposition_table_probe before searching, transposition_table_store after evaluation
 *  5. Call transposition_table_free() at program exit
 */

#ifndef TRANSPOSITION_H
#define TRANSPOSITION_H

#include <stdint.h>
#include <stddef.h>
#include "../TicTacToe/tic_tac_toe.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Initialize Zobrist random keys.
 * Must be called once at program start before any hashing operations.
 */
void zobrist_init(void);

/**
 * Compute full board hash from scratch.
 *
 * Parameters:
 *  - board: Current position
 *  - aiPlayer: The maximizing player ('x' or 'o')
 *
 * Returns: 64-bit Zobrist hash uniquely identifying this position
 */
uint64_t zobrist_hash(char board[BOARD_SIZE][BOARD_SIZE], char aiPlayer);

/**
 * Incremental hash update: toggle a piece on/off.
 *
 * Usage:
 *  - Before move: new_hash = zobrist_toggle(hash, row, col, player)
 *  - After unmake: hash = zobrist_toggle(new_hash, row, col, player)
 *
 * Parameters:
 *  - hash: Current position hash
 *  - row, col: Cell coordinates (0-based)
 *  - player: Piece to toggle ('x' or 'o')
 *
 * Returns: Updated hash after toggling the piece
 */
uint64_t zobrist_toggle(uint64_t hash, int row, int col, char player);

/**
 * Transposition table entry types (bound classification)
 */
typedef enum
{
    TRANSPOSITION_TABLE_EXACT,       /* Exact score (PV node: alpha < score < beta) */
    TRANSPOSITION_TABLE_LOWERBOUND,  /* Score >= stored value (beta cutoff: score >= beta) */
    TRANSPOSITION_TABLE_UPPERBOUND   /* Score <= stored value (alpha cutoff: score <= alpha) */
} TranspositionTableNodeType;

/**
 * Transposition table entry.
 * Stores search results for a single position.
 *
 * Size: Typically 16 bytes on 64-bit systems with standard alignment.
 * (Compiler-dependent; use sizeof(TranspositionTableEntry) for exact size)
 */
typedef struct
{
    uint64_t hash;      /* Zobrist hash (0 = empty slot) */
    int16_t score;      /* Stored score */
    uint16_t depth;     /* Search depth (supports boards up to 255×255) */
    uint8_t type;       /* TranspositionTableNodeType */
    int8_t best_row;    /* Best move row (-1 if none) */
    int8_t best_col;    /* Best move col (-1 if none) */
    uint8_t padding;    /* Padding for alignment */
} TranspositionTableEntry;

/**
 * Initialize transposition table with given size.
 *
 * Parameters:
 *  - size: Number of entries (e.g., 1000000 for ~16 MB)
 *
 * Note: Call transposition_table_free() when done to release memory
 */
void transposition_table_init(size_t size);

/**
 * Free transposition table memory.
 * Safe to call even if transposition_table_init() was never called.
 */
void transposition_table_free(void);

/**
 * Probe transposition table for a usable cached result.
 *
 * Parameters:
 *  - hash: Position hash to look up
 *  - depth: Current search depth
 *  - alpha, beta: Current alpha-beta bounds
 *  - out_score: Output pointer for retrieved score (if found)
 *  - out_type: Output pointer for node type (if found)
 *
 * Returns:
 *  - 1 if a usable entry was found (out_score/out_type are valid)
 *  - 0 otherwise (cache miss, collision, or insufficient depth)
 */
int transposition_table_probe(uint64_t hash, int depth, int alpha, int beta,
                               int *out_score, TranspositionTableNodeType *out_type);

/**
 * Store position evaluation in transposition table.
 *
 * Parameters:
 *  - hash: Position hash
 *  - depth: Search depth
 *  - score: Evaluated score
 *  - type: Node type (exact/lower/upper bound)
 *  - best_row, best_col: Best move found (-1 if none)
 */
void transposition_table_store(uint64_t hash, int depth, int score, TranspositionTableNodeType type,
                                int best_row, int best_col);

/**
 * Get transposition table statistics.
 *
 * Parameters:
 *  - hits: Number of successful probes
 *  - misses: Number of failed probes
 *  - collisions: Number of hash collisions
 */
void transposition_table_get_stats(size_t *hits, size_t *misses, size_t *collisions);

#ifdef __cplusplus
}
#endif

#endif
