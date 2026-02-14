#include "unity/unity.h"
#include "../src/MiniMax/mini_max.h"
#include "../src/MiniMax/transposition.h"
#include "../src/TicTacToe/tic_tac_toe.h"

// Test empty board plays center
void test_empty_board_plays_center(void)
{
    init_win_masks();
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
    init_win_masks();
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
    init_win_masks();
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

void test_minimax_suite(void)
{
    RUN_TEST(test_empty_board_plays_center);
    RUN_TEST(test_terminal_board_returns_invalid);
    RUN_TEST(test_overlapping_pieces_rejected);
}
