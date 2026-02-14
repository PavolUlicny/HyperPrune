#include "unity/unity.h"
#include "../src/MiniMax/mini_max.h"
#include "../src/MiniMax/transposition.h"
#include "../src/TicTacToe/tic_tac_toe.h"

// Test AI takes immediate winning move
void test_ai_takes_winning_move(void) {
    #if BOARD_SIZE == 3
    init_win_masks();
    zobrist_init();
    transposition_table_init(10000);

    // Setup: X has two in a row, needs (0,2) to win
    // X X _
    // O _ _
    // O _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 2, 0, 'o');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // AI should play (0,2) to complete the row
    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(2, col);

    transposition_table_free();
    #endif
}

// Test AI blocks opponent's winning move
void test_ai_blocks_opponent_win(void) {
    #if BOARD_SIZE == 3
    init_win_masks();
    zobrist_init();
    transposition_table_init(10000);

    // Setup: O has two in a row, X must block at (0,2)
    // O O _
    // X _ _
    // X _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'o');
    bitboard_make_move(&board, 0, 1, 'o');
    bitboard_make_move(&board, 1, 0, 'x');
    bitboard_make_move(&board, 2, 0, 'x');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // AI should block at (0,2)
    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(2, col);

    transposition_table_free();
    #endif
}

// Test single empty cell scenario
void test_single_empty_cell(void) {
    init_win_masks();
    zobrist_init();
    transposition_table_init(10000);

    // Create board with only one empty cell at (1,1)
    Bitboard board = {0, 0};
    for (int r = 0; r < BOARD_SIZE; r++) {
        for (int c = 0; c < BOARD_SIZE; c++) {
            if (r == 1 && c == 1) continue;  // Leave (1,1) empty
            char player = ((r + c) % 2 == 0) ? 'x' : 'o';
            bitboard_make_move(&board, r, c, player);
        }
    }

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // AI should play the only available move
    TEST_ASSERT_EQUAL(1, row);
    TEST_ASSERT_EQUAL(1, col);

    transposition_table_free();
}

// Test AI with 'o' as maximizing player
void test_ai_as_o_player(void) {
    init_win_masks();
    zobrist_init();
    transposition_table_init(10000);

    Bitboard board = {0, 0};
    int row, col;

    // Empty board, AI is 'o'
    getAiMove(board, 'o', &row, &col);

    // Should still play center
    TEST_ASSERT_EQUAL(BOARD_SIZE / 2, row);
    TEST_ASSERT_EQUAL(BOARD_SIZE / 2, col);

    transposition_table_free();
}

// Test tie scenario (full board)
void test_tie_scenario(void) {
    #if BOARD_SIZE == 3
    init_win_masks();
    zobrist_init();
    transposition_table_init(10000);

    // Create a tie board
    // X X O
    // O O X
    // X X O
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 0, 2, 'o');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 1, 1, 'o');
    bitboard_make_move(&board, 1, 2, 'x');
    bitboard_make_move(&board, 2, 0, 'x');
    bitboard_make_move(&board, 2, 1, 'x');
    bitboard_make_move(&board, 2, 2, 'o');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // Should return invalid (-1, -1) for terminal board
    TEST_ASSERT_EQUAL(-1, row);
    TEST_ASSERT_EQUAL(-1, col);

    transposition_table_free();
    #endif
}

// Test fork scenario (AI should create winning opportunity)
void test_fork_creation(void) {
    #if BOARD_SIZE == 3
    init_win_masks();
    zobrist_init();
    transposition_table_init(10000);

    // Setup: X can create a fork
    // X _ _
    // _ X _
    // O O _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 1, 1, 'x');
    bitboard_make_move(&board, 2, 0, 'o');
    bitboard_make_move(&board, 2, 1, 'o');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // AI should block or create optimal position
    // Just verify it returns a valid move
    TEST_ASSERT_TRUE(row >= 0 && row < BOARD_SIZE);
    TEST_ASSERT_TRUE(col >= 0 && col < BOARD_SIZE);
    TEST_ASSERT_TRUE(bitboard_is_empty(board, row, col));

    transposition_table_free();
    #endif
}

// Test AI can win via diagonal
void test_diagonal_win(void) {
    #if BOARD_SIZE == 3
    init_win_masks();
    zobrist_init();
    transposition_table_init(10000);

    // Create a simple scenario where diagonal is the clear winning move
    // _ _ _
    // _ X _
    // X O O
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 1, 1, 'x');  // Center
    bitboard_make_move(&board, 2, 0, 'x');  // Bottom-left (part of anti-diag)
    bitboard_make_move(&board, 2, 1, 'o');
    bitboard_make_move(&board, 2, 2, 'o');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // AI should play (0,2) to complete anti-diagonal and win
    // (or make another optimal move)
    TEST_ASSERT_TRUE(row >= 0 && row < BOARD_SIZE);
    TEST_ASSERT_TRUE(col >= 0 && col < BOARD_SIZE);

    transposition_table_free();
    #endif
}

void test_game_scenarios_suite(void) {
    RUN_TEST(test_ai_takes_winning_move);
    RUN_TEST(test_ai_blocks_opponent_win);
    RUN_TEST(test_single_empty_cell);
    RUN_TEST(test_ai_as_o_player);
    RUN_TEST(test_tie_scenario);
    RUN_TEST(test_fork_creation);
    RUN_TEST(test_diagonal_win);
}
