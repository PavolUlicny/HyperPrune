#include "unity/unity.h"
#include "../src/MiniMax/transposition.h"
#include "../src/TicTacToe/tic_tac_toe.h"

// Test incremental hash matches full hash
void test_incremental_hash_matches_full(void) {
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    uint64_t full_hash = zobrist_hash(board, 'x');

    // Make moves incrementally
    uint64_t incremental_hash = zobrist_hash(board, 'x');
    incremental_hash = zobrist_toggle(incremental_hash, 0, 0, 'x');
    bitboard_make_move(&board, 0, 0, 'x');

    incremental_hash = zobrist_toggle(incremental_hash, 1, 1, 'o');
    bitboard_make_move(&board, 1, 1, 'o');

    // Recompute full hash
    full_hash = zobrist_hash(board, 'x');

    TEST_ASSERT_EQUAL_UINT64(full_hash, incremental_hash);
}

// Test turn toggle produces different hashes
void test_turn_toggle_changes_hash(void) {
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    uint64_t hash1 = zobrist_hash(board, 'x');
    uint64_t hash2 = zobrist_toggle_turn(hash1);

    TEST_ASSERT_NOT_EQUAL_UINT64(hash1, hash2);

    // Toggle again returns original
    uint64_t hash3 = zobrist_toggle_turn(hash2);
    TEST_ASSERT_EQUAL_UINT64(hash1, hash3);
}

// Test same position produces same hash
void test_same_position_same_hash(void) {
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board1 = {0, 0};
    bitboard_make_move(&board1, 0, 0, 'x');
    bitboard_make_move(&board1, 1, 1, 'o');

    Bitboard board2 = {0, 0};
    bitboard_make_move(&board2, 1, 1, 'o');  // Different order
    bitboard_make_move(&board2, 0, 0, 'x');

    uint64_t hash1 = zobrist_hash(board1, 'x');
    uint64_t hash2 = zobrist_hash(board2, 'x');

    TEST_ASSERT_EQUAL_UINT64(hash1, hash2);
}

void test_zobrist_suite(void) {
    RUN_TEST(test_incremental_hash_matches_full);
    RUN_TEST(test_turn_toggle_changes_hash);
    RUN_TEST(test_same_position_same_hash);
}
