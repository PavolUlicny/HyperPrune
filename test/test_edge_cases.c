#include "unity/unity.h"
#include "../src/TicTacToe/tic_tac_toe.h"
#include "../src/MiniMax/mini_max.h"
#include "../src/MiniMax/transposition.h"

// Test bitboard_did_last_move_win for row win
void test_did_last_move_win_row(void)
{
    init_win_masks();
    Bitboard board = {0, 0};

    // Create row win at row 0
    for (int c = 0; c < BOARD_SIZE; c++)
    {
        bitboard_make_move(&board, 0, c, 'x');
    }

    // Check that last move at (0, BOARD_SIZE-1) wins
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.x_pieces, 0, BOARD_SIZE - 1));

    // Check that same config for a different cell also detects win
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.x_pieces, 0, 0));
}

// Test bitboard_did_last_move_win for column win
void test_did_last_move_win_col(void)
{
    init_win_masks();
    Bitboard board = {0, 0};

    // Create column win at col 0
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        bitboard_make_move(&board, r, 0, 'o');
    }

    // Check that last move at (BOARD_SIZE-1, 0) wins
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.o_pieces, BOARD_SIZE - 1, 0));
}

// Test bitboard_did_last_move_win for main diagonal
void test_did_last_move_win_main_diagonal(void)
{
    init_win_masks();
    Bitboard board = {0, 0};

    // Create main diagonal win
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        bitboard_make_move(&board, i, i, 'x');
    }

    // Check center of diagonal
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.x_pieces, BOARD_SIZE / 2, BOARD_SIZE / 2));
}

// Test bitboard_did_last_move_win for anti-diagonal (comprehensive - all positions)
void test_did_last_move_win_anti_diagonal(void)
{
#if BOARD_SIZE == 3
    init_win_masks();
    Bitboard board = {0, 0};

    // Create anti-diagonal win
    bitboard_make_move(&board, 0, 2, 'x');
    bitboard_make_move(&board, 1, 1, 'x');
    bitboard_make_move(&board, 2, 0, 'x');

    // All three positions should detect the anti-diagonal win
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.x_pieces, 0, 2));
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.x_pieces, 1, 1));
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.x_pieces, 2, 0));
#endif
}

// Test bitboard_did_last_move_win returns false for non-winning board
void test_did_last_move_win_no_win(void)
{
    init_win_masks();
    Bitboard board = {0, 0};

    // Partial row (not a win)
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 0, 1, 'x');

    TEST_ASSERT_FALSE(bitboard_did_last_move_win(board.x_pieces, 0, 1));
}

// Test bitboard_has_won returns false for empty board
void test_has_won_empty_board(void)
{
    init_win_masks();
    Bitboard board = {0, 0};

    TEST_ASSERT_FALSE(bitboard_has_won(board.x_pieces));
    TEST_ASSERT_FALSE(bitboard_has_won(board.o_pieces));
}

// Test bitboard_has_won returns false for partial board
void test_has_won_partial_board(void)
{
    init_win_masks();
    Bitboard board = {0, 0};

    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 1, 1, 'x');
    bitboard_make_move(&board, 0, 1, 'o');

    TEST_ASSERT_FALSE(bitboard_has_won(board.x_pieces));
    TEST_ASSERT_FALSE(bitboard_has_won(board.o_pieces));
}

// Test multiple make moves without unmake
void test_multiple_makes(void)
{
    Bitboard board = {0, 0};

    // Make multiple moves
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 1, 1, 'o');
    bitboard_make_move(&board, 2, 2, 'x');

    // Verify all are set
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, 0, 0));
    TEST_ASSERT_EQUAL('o', bitboard_get_cell(board, 1, 1));
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, 2, 2));
}

// Test unmake without prior make (should result in no change)
void test_unmake_empty_cell(void)
{
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');

    // Unmake a cell that was never set
    bitboard_unmake_move(&board, 1, 1, 'o');

    // Board should be unchanged except for (0,0)
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, 0, 0));
    TEST_ASSERT_EQUAL(' ', bitboard_get_cell(board, 1, 1));
}

// Test both players having pieces
void test_both_players_pieces(void)
{
    Bitboard board = {0, 0};

    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 1, 1, 'o');

    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, 0, 0));
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, 0, 1));
    TEST_ASSERT_EQUAL('o', bitboard_get_cell(board, 1, 0));
    TEST_ASSERT_EQUAL('o', bitboard_get_cell(board, 1, 1));
}

// Test all cells on empty board are empty
void test_all_cells_empty(void)
{
    Bitboard board = {0, 0};

    for (int r = 0; r < BOARD_SIZE; r++)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            TEST_ASSERT_TRUE(bitboard_is_empty(board, r, c));
            TEST_ASSERT_EQUAL(' ', bitboard_get_cell(board, r, c));
        }
    }
}

// Test corner cells
void test_corner_cells(void)
{
    Bitboard board = {0, 0};

    // Top-left
    bitboard_make_move(&board, 0, 0, 'x');
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, 0, 0));

    // Top-right
    bitboard_make_move(&board, 0, BOARD_SIZE - 1, 'o');
    TEST_ASSERT_EQUAL('o', bitboard_get_cell(board, 0, BOARD_SIZE - 1));

    // Bottom-left
    bitboard_make_move(&board, BOARD_SIZE - 1, 0, 'x');
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, BOARD_SIZE - 1, 0));

    // Bottom-right
    bitboard_make_move(&board, BOARD_SIZE - 1, BOARD_SIZE - 1, 'o');
    TEST_ASSERT_EQUAL('o', bitboard_get_cell(board, BOARD_SIZE - 1, BOARD_SIZE - 1));
}

// Test make/unmake cycle preserves hash
void test_make_unmake_hash_cycle(void)
{
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    uint64_t original_hash = zobrist_hash(board, 'x');

    // Make move and hash
    bitboard_make_move(&board, 1, 1, 'x');
    uint64_t new_hash = zobrist_toggle(original_hash, 1, 1, 'x');

    // Unmake and restore hash
    bitboard_unmake_move(&board, 1, 1, 'x');
    uint64_t restored_hash = zobrist_toggle(new_hash, 1, 1, 'x');

    TEST_ASSERT_EQUAL_UINT64(original_hash, restored_hash);
}

// Test zobrist hash is zero only when explicitly all keys sum to zero (extremely rare)
void test_zobrist_hash_nonzero(void)
{
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board, 'x');

    // Hash of empty board with player key should be non-zero
    // (Statistical property - not guaranteed but extremely likely)
    TEST_ASSERT_NOT_EQUAL_UINT64(0, hash);
}

void test_edge_cases_suite(void)
{
    RUN_TEST(test_did_last_move_win_row);
    RUN_TEST(test_did_last_move_win_col);
    RUN_TEST(test_did_last_move_win_main_diagonal);
    RUN_TEST(test_did_last_move_win_anti_diagonal);
    RUN_TEST(test_did_last_move_win_no_win);
    RUN_TEST(test_has_won_empty_board);
    RUN_TEST(test_has_won_partial_board);
    RUN_TEST(test_multiple_makes);
    RUN_TEST(test_unmake_empty_cell);
    RUN_TEST(test_both_players_pieces);
    RUN_TEST(test_all_cells_empty);
    RUN_TEST(test_corner_cells);
    RUN_TEST(test_make_unmake_hash_cycle);
    RUN_TEST(test_zobrist_hash_nonzero);
}
