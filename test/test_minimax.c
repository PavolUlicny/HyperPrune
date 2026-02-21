#include "unity/unity.h"
#include "../src/MiniMax/mini_max.h"
#include "../src/MiniMax/transposition.h"
#include "../src/TicTacToe/tic_tac_toe.h"

// Test empty board plays center
void test_empty_board_plays_center(void)
{
    zobrist_init();
    transposition_table_init(10000);

    Bitboard board = {0, 0};
    int row, col;

    getAiMove(board, 'x', &row, &col);

    TEST_ASSERT_EQUAL(BOARD_SIZE / 2, row);
    TEST_ASSERT_EQUAL(BOARD_SIZE / 2, col);

    transposition_table_free();
}

// Test terminal board returns invalid
void test_terminal_board_returns_invalid(void)
{
    zobrist_init();
    transposition_table_init(10000);

    // Create winning board for X (row 0)
    Bitboard board = {0, 0};
    for (int c = 0; c < BOARD_SIZE; c++)
    {
        bitboard_make_move(&board, 0, c, 'x');
    }

    int row, col;
    getAiMove(board, 'o', &row, &col);

    TEST_ASSERT_EQUAL(-1, row);
    TEST_ASSERT_EQUAL(-1, col);

    transposition_table_free();
}

// Test overlapping pieces rejected
void test_overlapping_pieces_rejected(void)
{
    zobrist_init();
    transposition_table_init(10000);

    // Create invalid board (overlapping at 0,0)
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 0, 0, 'o'); // Overlap!

    int row, col;
    getAiMove(board, 'x', &row, &col);

    TEST_ASSERT_EQUAL(-1, row);
    TEST_ASSERT_EQUAL(-1, col);

    transposition_table_free();
}

// Test terminal board returns invalid when AI ('x') has already won
void test_terminal_ai_x_wins(void)
{
    zobrist_init();
    transposition_table_init(10000);

    // X has won (row 0)
    Bitboard board = {0, 0};
    for (int c = 0; c < BOARD_SIZE; c++)
    {
        bitboard_make_move(&board, 0, c, 'x');
    }

    int row, col;
    getAiMove(board, 'x', &row, &col);

    TEST_ASSERT_EQUAL(-1, row);
    TEST_ASSERT_EQUAL(-1, col);

    transposition_table_free();
}

// Test terminal board returns invalid when AI ('o') has already won
void test_terminal_ai_o_wins(void)
{
    zobrist_init();
    transposition_table_init(10000);

    // O has won (row 0)
    Bitboard board = {0, 0};
    for (int c = 0; c < BOARD_SIZE; c++)
    {
        bitboard_make_move(&board, 0, c, 'o');
    }

    int row, col;
    getAiMove(board, 'o', &row, &col);

    TEST_ASSERT_EQUAL(-1, row);
    TEST_ASSERT_EQUAL(-1, col);

    transposition_table_free();
}

// Test terminal board returns invalid when opponent ('o') has won and AI is 'x'
void test_terminal_opponent_o_wins(void)
{
    zobrist_init();
    transposition_table_init(10000);

    // O has won (row 0)
    Bitboard board = {0, 0};
    for (int c = 0; c < BOARD_SIZE; c++)
    {
        bitboard_make_move(&board, 0, c, 'o');
    }

    int row, col;
    getAiMove(board, 'x', &row, &col);

    TEST_ASSERT_EQUAL(-1, row);
    TEST_ASSERT_EQUAL(-1, col);

    transposition_table_free();
}

// Test AI completes a column win (all existing win tests use rows).
void test_getAiMove_column_win(void)
{
#if BOARD_SIZE == 3
    zobrist_init();
    transposition_table_init(10000);

    // X needs (2,0) to complete column 0
    // X _ O
    // X O _
    // _ _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 1, 0, 'x');
    bitboard_make_move(&board, 0, 2, 'o');
    bitboard_make_move(&board, 1, 1, 'o');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    TEST_ASSERT_EQUAL(2, row);
    TEST_ASSERT_EQUAL(0, col);

    transposition_table_free();
#elif BOARD_SIZE == 4
    zobrist_init();
    transposition_table_init(10000);

    // X needs (3,0) to complete column 0
    // X _ O _
    // X O _ _
    // X _ _ _
    // _ _ _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 1, 0, 'x');
    bitboard_make_move(&board, 2, 0, 'x');
    bitboard_make_move(&board, 0, 2, 'o');
    bitboard_make_move(&board, 1, 1, 'o');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    TEST_ASSERT_EQUAL(3, row);
    TEST_ASSERT_EQUAL(0, col);

    transposition_table_free();
#endif
}

// Test AI blocks opponent from completing a column (all existing block tests use rows).
void test_getAiMove_blocks_column(void)
{
#if BOARD_SIZE == 3
    zobrist_init();
    transposition_table_init(10000);

    // O needs (2,0) to complete column 0, X must block
    // O X _
    // O _ X
    // _ _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'o');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 1, 2, 'x');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    TEST_ASSERT_EQUAL(2, row);
    TEST_ASSERT_EQUAL(0, col);

    transposition_table_free();
#elif BOARD_SIZE == 4
    zobrist_init();
    transposition_table_init(10000);

    // O needs (3,0) to complete column 0, X must block
    // O X _ _
    // O _ X _
    // O _ _ _
    // _ _ _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'o');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 2, 0, 'o');
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 1, 2, 'x');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    TEST_ASSERT_EQUAL(3, row);
    TEST_ASSERT_EQUAL(0, col);

    transposition_table_free();
#endif
}

// Test getAiMove returns the correct move even when the transposition table
// is completely disabled (size=0). Verifies the search itself is correct
// independent of the TT. Uses a board distinct from test_ai_takes_winning_move:
// X occupies the right end of row 0 and needs the left-end cell (0,0) to win.
void test_getAiMove_no_tt(void)
{
    zobrist_init();
    transposition_table_init(0); // Disabled

#if BOARD_SIZE == 3
    // X needs (0,0) to complete row 0
    // _ X X
    // _ O _
    // _ O _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 0, 2, 'x');
    bitboard_make_move(&board, 1, 1, 'o');
    bitboard_make_move(&board, 2, 1, 'o');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(0, col);
#elif BOARD_SIZE == 4
    // X needs (0,0) to complete row 0
    // _ X X X
    // O O _ _
    // _ _ _ _
    // _ _ _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 0, 2, 'x');
    bitboard_make_move(&board, 0, 3, 'x');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 1, 1, 'o');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(0, col);
#endif

    transposition_table_free();
}

void test_minimax_suite(void)
{
    RUN_TEST(test_empty_board_plays_center);
    RUN_TEST(test_terminal_board_returns_invalid);
    RUN_TEST(test_overlapping_pieces_rejected);
    RUN_TEST(test_terminal_ai_x_wins);
    RUN_TEST(test_terminal_ai_o_wins);
    RUN_TEST(test_terminal_opponent_o_wins);
    RUN_TEST(test_getAiMove_column_win);
    RUN_TEST(test_getAiMove_blocks_column);
    RUN_TEST(test_getAiMove_no_tt);
}
