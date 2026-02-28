#include "unity/unity.h"
#include "../src/Negamax/transposition.h"
#include "../src/TicTacToe/tic_tac_toe.h"

// Test transposition table store and probe
void test_tt_store_and_probe(void)
{
    zobrist_set_seed(42);
    zobrist_init();
    transposition_table_init(1000);

    Bitboard board = {0, 0};
    bitboard_make_move_rc(&board, 0, 0, 'x');
    uint64_t hash = zobrist_hash(board);

    // Store exact score
    transposition_table_store(hash, 50, TRANSPOSITION_TABLE_EXACT);

    // Probe should succeed
    int score;
    int found = transposition_table_probe(hash, 100, &score);

    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(50, score);

    transposition_table_free();
}

// Test transposition table with NULL table
void test_tt_null_table(void)
{
    // Initialize zobrist but don't initialize table
    zobrist_init();
    transposition_table_free();

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board);

    // Store should not crash
    transposition_table_store(hash, 50, TRANSPOSITION_TABLE_EXACT);

    // Probe should return 0 (miss)
    int score;
    int found = transposition_table_probe(hash, 100, &score);

    TEST_ASSERT_EQUAL(0, found);
}

// Test transposition table with size 0
void test_tt_zero_size(void)
{
    zobrist_init();
    transposition_table_init(0);

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board);

    // Operations should not crash when TT is disabled
    // Size 0 now properly disables TT (no allocation)
    transposition_table_store(hash, 50, TRANSPOSITION_TABLE_EXACT);

    int score;
    int found = transposition_table_probe(hash, 100, &score);

    // With size 0, TT is disabled, so probe should return 0 (not found)
    TEST_ASSERT_EQUAL(0, found);

    transposition_table_free();
}

// Test transposition table reinitialization
void test_tt_reinitialization(void)
{
    zobrist_init();
    transposition_table_init(1000);

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board);
    transposition_table_store(hash, 50, TRANSPOSITION_TABLE_EXACT);

    // Reinitialize should clear table
    transposition_table_init(1000);

    int score;
    int found = transposition_table_probe(hash, 100, &score);

    TEST_ASSERT_EQUAL(0, found);

    transposition_table_free();
}

// Test transposition table lowerbound cutoff
void test_tt_lowerbound_cutoff(void)
{
    zobrist_init();
    transposition_table_init(1000);

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board);

    // Store lowerbound score of 60
    transposition_table_store(hash, 60, TRANSPOSITION_TABLE_LOWERBOUND);

    // Probe with beta=50: should cutoff (score >= beta)
    int score;
    int found = transposition_table_probe(hash, 50, &score);

    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(60, score);

    // Probe with beta=70: should NOT cutoff (score < beta)
    found = transposition_table_probe(hash, 70, &score);
    TEST_ASSERT_EQUAL(0, found);

    transposition_table_free();
}

// Test hash collision handling
void test_tt_hash_collision(void)
{
    zobrist_init();
    transposition_table_init(1000);

    Bitboard board1 = {0, 0};
    bitboard_make_move_rc(&board1, 0, 0, 'x');
    uint64_t hash1 = zobrist_hash(board1);

    Bitboard board2 = {0, 0};
    bitboard_make_move_rc(&board2, 1, 1, 'o');
    uint64_t hash2 = zobrist_hash(board2);

    // Store first position
    transposition_table_store(hash1, 50, TRANSPOSITION_TABLE_EXACT);

    // Store second position (may collide or replace)
    transposition_table_store(hash2, 75, TRANSPOSITION_TABLE_EXACT);

    // Probe second should always work
    int score;
    int found = transposition_table_probe(hash2, 100, &score);

    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(75, score);

    transposition_table_free();
}

// Test storage layer round-trip fidelity at int16_t type limits.
// INT16_MIN/MAX are not scores the game engine ever produces (valid range is
// [-WIN, WIN] = [-100, 100]), but the TT stores scores as int16_t. This test
// confirms the narrowing cast in transposition_table_store() does not corrupt
// values at the extremes of the storage type.
void test_tt_score_boundaries(void)
{
    zobrist_init();
    transposition_table_init(1000);

    uint64_t hash = 12345;

    // Test INT16_MIN boundary
    transposition_table_store(hash, INT16_MIN, TRANSPOSITION_TABLE_EXACT);
    int score;
    int found = transposition_table_probe(hash, INT16_MAX, &score);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(INT16_MIN, score);

    // Test INT16_MAX boundary
    transposition_table_store(hash + 1, INT16_MAX, TRANSPOSITION_TABLE_EXACT);
    found = transposition_table_probe(hash + 1, INT16_MAX + 1000, &score);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(INT16_MAX, score);

    transposition_table_free();
}

// Test LOWERBOUND equality cutoff: score == beta should cutoff (off-by-one check)
void test_tt_cutoff_equality(void)
{
    zobrist_init();
    transposition_table_init(1000);

    uint64_t hash = 12345;
    int score;

    // LOWERBOUND with score == beta should cutoff
    transposition_table_store(hash, 50, TRANSPOSITION_TABLE_LOWERBOUND);
    int found = transposition_table_probe(hash, 50, &score);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(50, score);

    transposition_table_free();
}

// Test multiple consecutive reinitializations (stress test)
void test_tt_multiple_reinit(void)
{
    zobrist_init();

    for (int i = 0; i < 10; i++)
    {
        transposition_table_init(1000);

        // Store something
        transposition_table_store(12345, i * 10, TRANSPOSITION_TABLE_EXACT);

        // Verify it's there
        int score;
        int found = transposition_table_probe(12345, 100, &score);
        TEST_ASSERT_EQUAL(1, found);
        TEST_ASSERT_EQUAL(i * 10, score);
    }

    transposition_table_free();
}

// Test that non-power-of-2 sizes are correctly rounded up (tests round_up_power_of_2 indirectly)
void test_tt_non_power_of_two_sizes(void)
{
    zobrist_init();
    int score;
    int found;

    // Size 3 should round up to 4
    transposition_table_init(3);
    transposition_table_store(11111, 10, TRANSPOSITION_TABLE_EXACT);
    found = transposition_table_probe(11111, 100, &score);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(10, score);
    transposition_table_free();

    // Size 7 should round up to 8
    transposition_table_init(7);
    transposition_table_store(22222, 20, TRANSPOSITION_TABLE_EXACT);
    found = transposition_table_probe(22222, 100, &score);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(20, score);
    transposition_table_free();

    // Size 1023 should round up to 1024
    transposition_table_init(1023);
    transposition_table_store(33333, 30, TRANSPOSITION_TABLE_EXACT);
    found = transposition_table_probe(33333, 100, &score);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(30, score);
    transposition_table_free();
}

// Test that EXACT node type fires even when the stored score is outside the
// current alpha-beta window. The probe logic must return EXACT unconditionally
// (unlike LOWERBOUND/UPPERBOUND which are conditional on score vs beta/alpha).
void test_tt_exact_fires_outside_window(void)
{
    zobrist_init();
    transposition_table_init(1000);

    uint64_t hash = 99999;
    transposition_table_store(hash, 10, TRANSPOSITION_TABLE_EXACT);

    int score;
    // score=10 is below alpha=20: EXACT must still fire
    int found = transposition_table_probe(hash, 100, &score);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(10, score);

    // score=10 is above beta=5: EXACT must still fire
    found = transposition_table_probe(hash, 5, &score);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(10, score);

    transposition_table_free();
}

// Test the always-replace collision strategy: a second store at the same
// table index evicts the first entry, making it a miss on probe.
void test_tt_always_replace_evicts_previous(void)
{
    zobrist_init();
    // Size 4 (already a power of 2): mask = 3
    // hash_a=1 -> index 1 & 3 = 1
    // hash_b=5 -> index 5 & 3 = 1  (same slot, different hash)
    transposition_table_init(4);

    uint64_t hash_a = 1;
    uint64_t hash_b = 5;

    transposition_table_store(hash_a, 42, TRANSPOSITION_TABLE_EXACT);

    int score;
    TEST_ASSERT_EQUAL(1, transposition_table_probe(hash_a, 100, &score));
    TEST_ASSERT_EQUAL(42, score);

    // Store hash_b at the same slot — evicts hash_a
    transposition_table_store(hash_b, 77, TRANSPOSITION_TABLE_EXACT);

    TEST_ASSERT_EQUAL(1, transposition_table_probe(hash_b, 100, &score));
    TEST_ASSERT_EQUAL(77, score);

    // hash_a must now be a miss (evicted)
    TEST_ASSERT_EQUAL(0, transposition_table_probe(hash_a, 100, &score));

    transposition_table_free();
}

// Test that hash value 0 is a valid, storable key.
// The 'occupied' flag exists specifically so that hash==0 is not treated as
// an empty-slot sentinel, preserving full 64-bit hash entropy.
void test_tt_hash_zero(void)
{
    zobrist_init();
    transposition_table_init(1000);

    transposition_table_store(0, 55, TRANSPOSITION_TABLE_EXACT);

    int score;
    int found = transposition_table_probe(0, 100, &score);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(55, score);

    transposition_table_free();
}

// Test single-entry table (size=1): mask=0, every hash maps to slot 0.
// Verifies round_up_power_of_2(1)==1 and that the table works at minimum size.
void test_tt_size_one(void)
{
    zobrist_init();
    transposition_table_init(1);

    uint64_t hash_a = 12345;
    uint64_t hash_b = 67890;

    transposition_table_store(hash_a, 30, TRANSPOSITION_TABLE_EXACT);

    int score;
    TEST_ASSERT_EQUAL(1, transposition_table_probe(hash_a, 100, &score));
    TEST_ASSERT_EQUAL(30, score);

    // hash_b also maps to slot 0, evicting hash_a
    transposition_table_store(hash_b, 60, TRANSPOSITION_TABLE_EXACT);

    TEST_ASSERT_EQUAL(1, transposition_table_probe(hash_b, 100, &score));
    TEST_ASSERT_EQUAL(60, score);
    TEST_ASSERT_EQUAL(0, transposition_table_probe(hash_a, 100, &score));

    transposition_table_free();
}

void test_transposition_table_suite(void)
{
    RUN_TEST(test_tt_store_and_probe);
    RUN_TEST(test_tt_null_table);
    RUN_TEST(test_tt_zero_size);
    RUN_TEST(test_tt_reinitialization);
    RUN_TEST(test_tt_lowerbound_cutoff);
    RUN_TEST(test_tt_hash_collision);
    RUN_TEST(test_tt_score_boundaries);
    RUN_TEST(test_tt_cutoff_equality);
    RUN_TEST(test_tt_multiple_reinit);
    RUN_TEST(test_tt_non_power_of_two_sizes);
    RUN_TEST(test_tt_exact_fires_outside_window);
    RUN_TEST(test_tt_always_replace_evicts_previous);
    RUN_TEST(test_tt_hash_zero);
    RUN_TEST(test_tt_size_one);
}
