#include "unity/unity.h"
#include "../src/MiniMax/transposition.h"
#include "../src/TicTacToe/tic_tac_toe.h"

// Test transposition table store and probe
void test_tt_store_and_probe(void)
{
    zobrist_set_seed(42);
    zobrist_init();
    transposition_table_init(1000);

    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    uint64_t hash = zobrist_hash(board, 'x');

    // Store exact score
    transposition_table_store(hash, 50, TRANSPOSITION_TABLE_EXACT);

    // Probe should succeed
    int score;
    TranspositionTableNodeType type;
    int found = transposition_table_probe(hash, -100, 100, &score, &type);

    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(50, score);
    TEST_ASSERT_EQUAL(TRANSPOSITION_TABLE_EXACT, type);

    transposition_table_free();
}

// Test transposition table with NULL table
void test_tt_null_table(void)
{
    // Initialize zobrist but don't initialize table
    zobrist_init();
    transposition_table_free();

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board, 'x');

    // Store should not crash
    transposition_table_store(hash, 50, TRANSPOSITION_TABLE_EXACT);

    // Probe should return 0 (miss)
    int score;
    TranspositionTableNodeType type;
    int found = transposition_table_probe(hash, -100, 100, &score, &type);

    TEST_ASSERT_EQUAL(0, found);
}

// Test transposition table with size 0
void test_tt_zero_size(void)
{
    zobrist_init();
    transposition_table_init(0);

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board, 'x');

    // Operations should not crash
    // Note: round_up_power_of_2(0) returns 1, so table has 1 entry
    // Store and probe should work (but with many collisions)
    transposition_table_store(hash, 50, TRANSPOSITION_TABLE_EXACT);

    int score;
    TranspositionTableNodeType type;
    int found = transposition_table_probe(hash, -100, 100, &score, &type);

    // With size rounded to 1, the probe should succeed
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(50, score);

    transposition_table_free();
}

// Test transposition table reinitialization
void test_tt_reinitialization(void)
{
    zobrist_init();
    transposition_table_init(1000);

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board, 'x');
    transposition_table_store(hash, 50, TRANSPOSITION_TABLE_EXACT);

    // Reinitialize should clear table
    transposition_table_init(1000);

    int score;
    TranspositionTableNodeType type;
    int found = transposition_table_probe(hash, -100, 100, &score, &type);

    TEST_ASSERT_EQUAL(0, found);

    transposition_table_free();
}

// Test transposition table lowerbound cutoff
void test_tt_lowerbound_cutoff(void)
{
    zobrist_init();
    transposition_table_init(1000);

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board, 'x');

    // Store lowerbound score of 60
    transposition_table_store(hash, 60, TRANSPOSITION_TABLE_LOWERBOUND);

    // Probe with beta=50: should cutoff (score >= beta)
    int score;
    TranspositionTableNodeType type;
    int found = transposition_table_probe(hash, -100, 50, &score, &type);

    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(60, score);

    // Probe with beta=70: should NOT cutoff (score < beta)
    found = transposition_table_probe(hash, -100, 70, &score, &type);
    TEST_ASSERT_EQUAL(0, found);

    transposition_table_free();
}

// Test transposition table upperbound cutoff
void test_tt_upperbound_cutoff(void)
{
    zobrist_init();
    transposition_table_init(1000);

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board, 'x');

    // Store upperbound score of 30
    transposition_table_store(hash, 30, TRANSPOSITION_TABLE_UPPERBOUND);

    // Probe with alpha=40: should cutoff (score <= alpha)
    int score;
    TranspositionTableNodeType type;
    int found = transposition_table_probe(hash, 40, 100, &score, &type);

    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(30, score);

    // Probe with alpha=20: should NOT cutoff (score > alpha)
    found = transposition_table_probe(hash, 20, 100, &score, &type);
    TEST_ASSERT_EQUAL(0, found);

    transposition_table_free();
}

// Test hash collision handling
void test_tt_hash_collision(void)
{
    zobrist_init();
    transposition_table_init(1000);

    Bitboard board1 = {0, 0};
    bitboard_make_move(&board1, 0, 0, 'x');
    uint64_t hash1 = zobrist_hash(board1, 'x');

    Bitboard board2 = {0, 0};
    bitboard_make_move(&board2, 1, 1, 'o');
    uint64_t hash2 = zobrist_hash(board2, 'x');

    // Store first position
    transposition_table_store(hash1, 50, TRANSPOSITION_TABLE_EXACT);

    // Store second position (may collide or replace)
    transposition_table_store(hash2, 75, TRANSPOSITION_TABLE_EXACT);

    // Probe second should always work
    int score;
    TranspositionTableNodeType type;
    int found = transposition_table_probe(hash2, -100, 100, &score, &type);

    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(75, score);

    transposition_table_free();
}

// Test zobrist_set_seed produces different keys
void test_zobrist_different_seeds(void)
{
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    uint64_t hash1 = zobrist_hash(board, 'x');

    // Reinitialize with different seed
    zobrist_set_seed(123);
    zobrist_init();

    uint64_t hash2 = zobrist_hash(board, 'x');

    TEST_ASSERT_NOT_EQUAL_UINT64(hash1, hash2);
}

// Test different aiPlayer produces different hashes
void test_zobrist_different_aiplayer(void)
{
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 1, 1, 'o');

    uint64_t hash_x = zobrist_hash(board, 'x');
    uint64_t hash_o = zobrist_hash(board, 'o');

    // Same position, different maximizing player -> different hash
    TEST_ASSERT_NOT_EQUAL_UINT64(hash_x, hash_o);
}

// Test INT16_MIN/MAX score boundaries (cast truncation check)
void test_tt_score_boundaries(void)
{
    zobrist_init();
    transposition_table_init(1000);

    uint64_t hash = 12345;

    // Test INT16_MIN boundary
    transposition_table_store(hash, INT16_MIN, TRANSPOSITION_TABLE_EXACT);
    int score;
    TranspositionTableNodeType type;
    int found = transposition_table_probe(hash, INT16_MIN - 1000, INT16_MAX, &score, &type);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(INT16_MIN, score);

    // Test INT16_MAX boundary
    transposition_table_store(hash + 1, INT16_MAX, TRANSPOSITION_TABLE_EXACT);
    found = transposition_table_probe(hash + 1, INT16_MIN, INT16_MAX + 1000, &score, &type);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(INT16_MAX, score);

    transposition_table_free();
}

// Test alpha/beta equality in cutoff conditions (off-by-one check)
void test_tt_cutoff_equality(void)
{
    zobrist_init();
    transposition_table_init(1000);

    uint64_t hash = 12345;
    int score;
    TranspositionTableNodeType type;

    // LOWERBOUND with score == beta should cutoff
    transposition_table_store(hash, 50, TRANSPOSITION_TABLE_LOWERBOUND);
    int found = transposition_table_probe(hash, -100, 50, &score, &type);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(50, score);

    // UPPERBOUND with score == alpha should cutoff
    transposition_table_store(hash + 1, 30, TRANSPOSITION_TABLE_UPPERBOUND);
    found = transposition_table_probe(hash + 1, 30, 100, &score, &type);
    TEST_ASSERT_EQUAL(1, found);
    TEST_ASSERT_EQUAL(30, score);

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
        TranspositionTableNodeType type;
        int found = transposition_table_probe(12345, -100, 100, &score, &type);
        TEST_ASSERT_EQUAL(1, found);
        TEST_ASSERT_EQUAL(i * 10, score);
    }

    transposition_table_free();
}

void test_transposition_table_suite(void)
{
    RUN_TEST(test_tt_store_and_probe);
    RUN_TEST(test_tt_null_table);
    RUN_TEST(test_tt_zero_size);
    RUN_TEST(test_tt_reinitialization);
    RUN_TEST(test_tt_lowerbound_cutoff);
    RUN_TEST(test_tt_upperbound_cutoff);
    RUN_TEST(test_tt_hash_collision);
    RUN_TEST(test_zobrist_different_seeds);
    RUN_TEST(test_zobrist_different_aiplayer);
    RUN_TEST(test_tt_score_boundaries);
    RUN_TEST(test_tt_cutoff_equality);
    RUN_TEST(test_tt_multiple_reinit);
}
