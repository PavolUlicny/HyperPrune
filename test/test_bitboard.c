#include "unity/unity.h"
#include "../src/TicTacToe/tic_tac_toe.h"

// Test all 8 win patterns on 3x3
void test_all_win_patterns(void) {
    init_win_masks();
    Bitboard board;

    // Test row wins (BOARD_SIZE patterns)
    for (int r = 0; r < BOARD_SIZE; r++) {
        board = (Bitboard){0, 0};
        for (int c = 0; c < BOARD_SIZE; c++) {
            bitboard_make_move(&board, r, c, 'x');
        }
        TEST_ASSERT_TRUE(bitboard_has_won(board.x_pieces));
    }

    // Test column wins (BOARD_SIZE patterns)
    for (int c = 0; c < BOARD_SIZE; c++) {
        board = (Bitboard){0, 0};
        for (int r = 0; r < BOARD_SIZE; r++) {
            bitboard_make_move(&board, r, c, 'x');
        }
        TEST_ASSERT_TRUE(bitboard_has_won(board.x_pieces));
    }

    // Test main diagonal
    board = (Bitboard){0, 0};
    for (int i = 0; i < BOARD_SIZE; i++) {
        bitboard_make_move(&board, i, i, 'x');
    }
    TEST_ASSERT_TRUE(bitboard_has_won(board.x_pieces));

    // Test anti-diagonal
    board = (Bitboard){0, 0};
    for (int i = 0; i < BOARD_SIZE; i++) {
        bitboard_make_move(&board, i, BOARD_SIZE - 1 - i, 'x');
    }
    TEST_ASSERT_TRUE(bitboard_has_won(board.x_pieces));
}

// Test make/unmake symmetry
void test_make_unmake_symmetry(void) {
    Bitboard board = {0, 0};
    Bitboard original = board;

    bitboard_make_move(&board, 1, 1, 'x');
    TEST_ASSERT_NOT_EQUAL(board.x_pieces, original.x_pieces);

    bitboard_unmake_move(&board, 1, 1, 'x');
    TEST_ASSERT_EQUAL(board.x_pieces, original.x_pieces);
    TEST_ASSERT_EQUAL(board.o_pieces, original.o_pieces);
}

// Test cell operations
void test_cell_operations(void) {
    Bitboard board = {0, 0};

    TEST_ASSERT_TRUE(bitboard_is_empty(board, 0, 0));
    TEST_ASSERT_EQUAL(' ', bitboard_get_cell(board, 0, 0));

    bitboard_make_move(&board, 0, 0, 'x');
    TEST_ASSERT_FALSE(bitboard_is_empty(board, 0, 0));
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, 0, 0));

    bitboard_make_move(&board, 1, 1, 'o');
    TEST_ASSERT_EQUAL('o', bitboard_get_cell(board, 1, 1));
}

void test_bitboard_suite(void) {
    RUN_TEST(test_all_win_patterns);
    RUN_TEST(test_make_unmake_symmetry);
    RUN_TEST(test_cell_operations);
}
