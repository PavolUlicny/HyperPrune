#include "unity/unity.h"
#include "../src/MiniMax/transposition.h"
#include "../src/TicTacToe/tic_tac_toe.h"

// Test incremental hash matches full hash
void test_incremental_hash_matches_full(void)
{
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
void test_turn_toggle_changes_hash(void)
{
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
void test_same_position_same_hash(void)
{
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board1 = {0, 0};
    bitboard_make_move(&board1, 0, 0, 'x');
    bitboard_make_move(&board1, 1, 1, 'o');

    Bitboard board2 = {0, 0};
    bitboard_make_move(&board2, 1, 1, 'o'); // Different order
    bitboard_make_move(&board2, 0, 0, 'x');

    uint64_t hash1 = zobrist_hash(board1, 'x');
    uint64_t hash2 = zobrist_hash(board2, 'x');

    TEST_ASSERT_EQUAL_UINT64(hash1, hash2);
}

// Test zobrist seed boundary values
void test_zobrist_seed_boundaries(void)
{
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');

    // Test seed 0
    zobrist_set_seed(0);
    zobrist_init();
    uint64_t hash_seed0 = zobrist_hash(board, 'x');

    // Test seed UINT64_MAX
    zobrist_set_seed(UINT64_MAX);
    zobrist_init();
    uint64_t hash_seedmax = zobrist_hash(board, 'x');

    // Different seeds should produce different hashes
    TEST_ASSERT_NOT_EQUAL_UINT64(hash_seed0, hash_seedmax);

    // Both hashes should be non-zero
    TEST_ASSERT_NOT_EQUAL_UINT64(0, hash_seed0);
    TEST_ASSERT_NOT_EQUAL_UINT64(0, hash_seedmax);
}

// Test zobrist toggle symmetry (repeated toggles)
void test_zobrist_toggle_symmetry(void)
{
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board, 'x');
    uint64_t original_hash = hash;

    // Toggle same cell 10 times (5 pairs)
    for (int i = 0; i < 5; i++)
    {
        hash = zobrist_toggle(hash, 1, 1, 'x');
        hash = zobrist_toggle(hash, 1, 1, 'x');
    }

    // Should return to original (XOR symmetry)
    TEST_ASSERT_EQUAL_UINT64(original_hash, hash);
}

// Test that zobrist_hash returns the same value when called twice on the same board.
// Ensures no internal state is mutated between calls.
void test_zobrist_hash_idempotent(void)
{
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 1, 1, 'o');

    uint64_t hash1 = zobrist_hash(board, 'x');
    uint64_t hash2 = zobrist_hash(board, 'x');

    TEST_ASSERT_EQUAL_UINT64(hash1, hash2);
}

// Test that the empty board hashes differently for 'x' vs 'o' as aiPlayer.
// On an empty board there are no piece keys — only the player perspective key
// differentiates them. This verifies the player key is included even with no pieces.
void test_zobrist_empty_board_different_aiplayer(void)
{
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard empty = {0, 0};
    uint64_t hash_x = zobrist_hash(empty, 'x');
    uint64_t hash_o = zobrist_hash(empty, 'o');

    TEST_ASSERT_NOT_EQUAL_UINT64(hash_x, hash_o);
}

// Test that the Zobrist key for an 'x' piece and an 'o' piece at the same cell
// are different. If they were equal, toggling either piece would produce identical
// hashes, making them indistinguishable in the transposition table.
void test_zobrist_x_and_o_keys_at_same_cell_differ(void)
{
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    uint64_t base = zobrist_hash(board, 'x');

    uint64_t with_x = zobrist_toggle(base, 0, 0, 'x');
    uint64_t with_o = zobrist_toggle(base, 0, 0, 'o');

    TEST_ASSERT_NOT_EQUAL_UINT64(with_x, with_o);
    TEST_ASSERT_NOT_EQUAL_UINT64(base, with_x);
    TEST_ASSERT_NOT_EQUAL_UINT64(base, with_o);
}

void test_zobrist_suite(void)
{
    RUN_TEST(test_incremental_hash_matches_full);
    RUN_TEST(test_turn_toggle_changes_hash);
    RUN_TEST(test_same_position_same_hash);
    RUN_TEST(test_zobrist_seed_boundaries);
    RUN_TEST(test_zobrist_toggle_symmetry);
    RUN_TEST(test_zobrist_hash_idempotent);
    RUN_TEST(test_zobrist_empty_board_different_aiplayer);
    RUN_TEST(test_zobrist_x_and_o_keys_at_same_cell_differ);
}
