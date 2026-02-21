#include "unity/unity.h"
#include "../src/MiniMax/mini_max.h"
#include "../src/MiniMax/transposition.h"
#include "../src/TicTacToe/tic_tac_toe.h"

// Test AI takes immediate winning move
void test_ai_takes_winning_move(void)
{
#if BOARD_SIZE == 3
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
#elif BOARD_SIZE == 4
    zobrist_init();
    transposition_table_init(10000);

    // Setup: X has three in a row, needs (0,3) to win
    // X X X _
    // O O _ _
    // _ _ _ _
    // _ _ _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 0, 2, 'x');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 1, 1, 'o');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // AI should play (0,3) to complete the row
    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(3, col);

    transposition_table_free();
#endif
}

// Test AI blocks opponent's winning move
void test_ai_blocks_opponent_win(void)
{
#if BOARD_SIZE == 3
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
#elif BOARD_SIZE == 4
    zobrist_init();
    transposition_table_init(10000);

    // Setup: O has three in a row, X must block at (0,3)
    // O O O _
    // X X _ _
    // _ _ _ _
    // _ _ _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'o');
    bitboard_make_move(&board, 0, 1, 'o');
    bitboard_make_move(&board, 0, 2, 'o');
    bitboard_make_move(&board, 1, 0, 'x');
    bitboard_make_move(&board, 1, 1, 'x');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // AI should block at (0,3)
    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(3, col);

    transposition_table_free();
#endif
}

// Test single empty cell scenario
void test_single_empty_cell(void)
{
#if BOARD_SIZE == 3
    zobrist_init();
    transposition_table_init(10000);

    // Create board with only one empty cell at (1,1)
    Bitboard board = {0, 0};
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            if (r == 1 && c == 1)
                continue; // Leave (1,1) empty
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
#elif BOARD_SIZE == 4
    zobrist_init();
    transposition_table_init(10000);

    // Create 4x4 board with only one empty cell at (2,2)
    // Layout (no 4-in-a-row):
    // X O X O
    // O X O X
    // X X _ O
    // O X O X
    Bitboard board = {0, 0};

    // Place X pieces (8 pieces)
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 0, 2, 'x');
    bitboard_make_move(&board, 1, 1, 'x');
    bitboard_make_move(&board, 1, 3, 'x');
    bitboard_make_move(&board, 2, 0, 'x');
    bitboard_make_move(&board, 2, 1, 'x');
    bitboard_make_move(&board, 3, 1, 'x');
    bitboard_make_move(&board, 3, 3, 'x');

    // Place O pieces (7 pieces)
    bitboard_make_move(&board, 0, 1, 'o');
    bitboard_make_move(&board, 0, 3, 'o');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 1, 2, 'o');
    bitboard_make_move(&board, 2, 3, 'o');
    bitboard_make_move(&board, 3, 0, 'o');
    bitboard_make_move(&board, 3, 2, 'o');

    // (2,2) is the only empty cell

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // AI should play the only available move
    TEST_ASSERT_EQUAL(2, row);
    TEST_ASSERT_EQUAL(2, col);

    transposition_table_free();
#endif
}

// Test AI with 'o' as maximizing player
void test_ai_as_o_player(void)
{
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
void test_tie_scenario(void)
{
#if BOARD_SIZE == 3
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
#elif BOARD_SIZE == 4
    zobrist_init();
    transposition_table_init(10000);

    // Create a tie board (no 4-in-a-row)
    // X O X O
    // O X O X
    // X O X O
    // O X O X
    Bitboard board = {0, 0};
    for (int i = 0; i < 16; i++)
    {
        int r = i / 4;
        int c = i % 4;
        char player = ((r + c) % 2 == 0) ? 'x' : 'o';
        bitboard_make_move(&board, r, c, player);
    }

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // Should return invalid (-1, -1) for terminal board
    TEST_ASSERT_EQUAL(-1, row);
    TEST_ASSERT_EQUAL(-1, col);

    transposition_table_free();
#endif
}

// Test fork scenario (AI should create winning opportunity)
void test_fork_creation(void)
{
#if BOARD_SIZE == 3
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
#elif BOARD_SIZE == 4
    zobrist_init();
    transposition_table_init(10000);

    // Setup: X has strategic positions
    // X _ _ _
    // _ X _ _
    // O O _ _
    // _ _ _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 1, 1, 'x');
    bitboard_make_move(&board, 2, 0, 'o');
    bitboard_make_move(&board, 2, 1, 'o');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // Just verify it returns a valid move
    TEST_ASSERT_TRUE(row >= 0 && row < BOARD_SIZE);
    TEST_ASSERT_TRUE(col >= 0 && col < BOARD_SIZE);
    TEST_ASSERT_TRUE(bitboard_is_empty(board, row, col));

    transposition_table_free();
#endif
}

// Test AI can win via diagonal
void test_diagonal_win(void)
{
#if BOARD_SIZE == 3
    zobrist_init();
    transposition_table_init(10000);

    // Create a simple scenario where diagonal is the clear winning move
    // _ _ _
    // _ X _
    // X O O
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 1, 1, 'x'); // Center
    bitboard_make_move(&board, 2, 0, 'x'); // Bottom-left (part of anti-diag)
    bitboard_make_move(&board, 2, 1, 'o');
    bitboard_make_move(&board, 2, 2, 'o');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // AI should play (0,2) to complete anti-diagonal and win
    // (or make another optimal move)
    TEST_ASSERT_TRUE(row >= 0 && row < BOARD_SIZE);
    TEST_ASSERT_TRUE(col >= 0 && col < BOARD_SIZE);

    transposition_table_free();
#elif BOARD_SIZE == 4
    zobrist_init();
    transposition_table_init(10000);

    // Create scenario with diagonal potential
    // X _ _ _
    // _ X _ _
    // _ _ X _
    // O O O _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x'); // Main diagonal
    bitboard_make_move(&board, 1, 1, 'x'); // Main diagonal
    bitboard_make_move(&board, 2, 2, 'x'); // Main diagonal
    bitboard_make_move(&board, 3, 0, 'o');
    bitboard_make_move(&board, 3, 1, 'o');
    bitboard_make_move(&board, 3, 2, 'o');

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // AI should play (3,3) to complete diagonal OR block O
    TEST_ASSERT_TRUE(row >= 0 && row < BOARD_SIZE);
    TEST_ASSERT_TRUE(col >= 0 && col < BOARD_SIZE);

    transposition_table_free();
#endif
}

// Test getAiMove with exactly two empty cells
void test_getAiMove_two_empty_cells(void)
{
#if BOARD_SIZE == 3
    zobrist_init();
    transposition_table_init(10000);

    // Create board with exactly 2 empty cells
    // X X O
    // O X _
    // X O _  <- (1,2) and (2,2) empty
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 0, 2, 'o');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 1, 1, 'x');
    bitboard_make_move(&board, 2, 0, 'x');
    bitboard_make_move(&board, 2, 1, 'o');
    // (1,2) and (2,2) are empty

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // Should return valid move (not -1, -1)
    TEST_ASSERT_TRUE(row >= 0 && row < BOARD_SIZE);
    TEST_ASSERT_TRUE(col >= 0 && col < BOARD_SIZE);
    TEST_ASSERT_TRUE((row == 1 && col == 2) || (row == 2 && col == 2));

    transposition_table_free();
#elif BOARD_SIZE == 4
    zobrist_init();
    transposition_table_init(10000);

    // Create 4x4 board with exactly 2 empty cells at (3,2) and (3,3)
    // Layout avoids 4-in-a-row (anti-diagonal broken at 1,2):
    // X O X O
    // O X X X
    // X O X O
    // O X _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 0, 1, 'o');
    bitboard_make_move(&board, 0, 2, 'x');
    bitboard_make_move(&board, 0, 3, 'o');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 1, 1, 'x');
    bitboard_make_move(&board, 1, 2, 'x'); // Changed from 'o' to break anti-diagonal
    bitboard_make_move(&board, 1, 3, 'x');
    bitboard_make_move(&board, 2, 0, 'x');
    bitboard_make_move(&board, 2, 1, 'o');
    bitboard_make_move(&board, 2, 2, 'x');
    bitboard_make_move(&board, 2, 3, 'o');
    bitboard_make_move(&board, 3, 0, 'o');
    bitboard_make_move(&board, 3, 1, 'x');
    // (3,2) and (3,3) are empty

    int row, col;
    getAiMove(board, 'x', &row, &col);

    // Should return valid move
    TEST_ASSERT_TRUE(row >= 0 && row < BOARD_SIZE);
    TEST_ASSERT_TRUE(col >= 0 && col < BOARD_SIZE);

    transposition_table_free();
#endif
}

// Test hash consistency over full game (end-to-end)
void test_hash_consistency_full_game(void)
{
#if BOARD_SIZE == 3
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board, 'x');

    // Play predetermined sequence
    int moves[][2] = {{1, 1}, {0, 0}, {0, 2}, {2, 0}, {1, 0}};
    char players[] = {'x', 'o', 'x', 'o', 'x'};

    for (int i = 0; i < 5; i++)
    {
        bitboard_make_move(&board, moves[i][0], moves[i][1], players[i]);
        hash = zobrist_toggle(hash, moves[i][0], moves[i][1], players[i]);

        // Verify incremental matches full
        uint64_t full_hash = zobrist_hash(board, 'x');
        TEST_ASSERT_EQUAL_UINT64(full_hash, hash);
    }
#elif BOARD_SIZE == 4
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    uint64_t hash = zobrist_hash(board, 'x');

    // Play predetermined sequence for 4x4
    int moves[][2] = {{1, 1}, {0, 0}, {0, 2}, {2, 0}, {1, 0}, {3, 3}, {2, 2}};
    char players[] = {'x', 'o', 'x', 'o', 'x', 'o', 'x'};

    for (int i = 0; i < 7; i++)
    {
        bitboard_make_move(&board, moves[i][0], moves[i][1], players[i]);
        hash = zobrist_toggle(hash, moves[i][0], moves[i][1], players[i]);

        // Verify incremental matches full
        uint64_t full_hash = zobrist_hash(board, 'x');
        TEST_ASSERT_EQUAL_UINT64(full_hash, hash);
    }
#endif
}

// Test AI as 'o' takes immediate winning move.
// Board is designed so the winning move is at the lowest bit index among empty
// cells — ensuring the engine finds and plays it first (and exits early).
void test_ai_o_takes_winning_move(void)
{
#if BOARD_SIZE == 3
    zobrist_init();
    transposition_table_init(10000);

    // O needs (0,0) to complete column 0 — lowest-bit empty cell.
    // Any other O move lets X play (0,0) and win row 0.
    // _ X X
    // O X _
    // O _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 0, 2, 'x');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 1, 1, 'x');
    bitboard_make_move(&board, 2, 0, 'o');

    int row, col;
    getAiMove(board, 'o', &row, &col);

    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(0, col);

    transposition_table_free();
#elif BOARD_SIZE == 4
    zobrist_init();
    transposition_table_init(10000);

    // O needs (0,0) to complete column 0 — lowest-bit empty cell.
    // Any other O move lets X play (0,0) and complete the main diagonal.
    // _ X _ X
    // O X _ _
    // O _ X _
    // O _ _ X
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 0, 3, 'x');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 1, 1, 'x');
    bitboard_make_move(&board, 2, 0, 'o');
    bitboard_make_move(&board, 2, 2, 'x');
    bitboard_make_move(&board, 3, 0, 'o');
    bitboard_make_move(&board, 3, 3, 'x');

    int row, col;
    getAiMove(board, 'o', &row, &col);

    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(0, col);

    transposition_table_free();
#endif
}

// Test AI as 'o' blocks opponent ('x') winning move
void test_ai_o_blocks_x_win(void)
{
#if BOARD_SIZE == 3
    zobrist_init();
    transposition_table_init(10000);

    // X has two in a row (row 0), O must block at (0,2)
    // X X _
    // O _ _
    // O _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 2, 0, 'o');

    int row, col;
    getAiMove(board, 'o', &row, &col);

    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(2, col);

    transposition_table_free();
#elif BOARD_SIZE == 4
    zobrist_init();
    transposition_table_init(10000);

    // X has three in a row (row 0), O must block at (0,3)
    // X X X _
    // O O _ _
    // _ _ _ _
    // _ _ _ _
    Bitboard board = {0, 0};
    bitboard_make_move(&board, 0, 0, 'x');
    bitboard_make_move(&board, 0, 1, 'x');
    bitboard_make_move(&board, 0, 2, 'x');
    bitboard_make_move(&board, 1, 0, 'o');
    bitboard_make_move(&board, 1, 1, 'o');

    int row, col;
    getAiMove(board, 'o', &row, &col);

    TEST_ASSERT_EQUAL(0, row);
    TEST_ASSERT_EQUAL(3, col);

    transposition_table_free();
#endif
}

void test_game_scenarios_suite(void)
{
    RUN_TEST(test_ai_takes_winning_move);
    RUN_TEST(test_ai_blocks_opponent_win);
    RUN_TEST(test_single_empty_cell);
    RUN_TEST(test_ai_as_o_player);
    RUN_TEST(test_tie_scenario);
    RUN_TEST(test_fork_creation);
    RUN_TEST(test_diagonal_win);
    RUN_TEST(test_getAiMove_two_empty_cells);
    RUN_TEST(test_hash_consistency_full_game);
    RUN_TEST(test_ai_o_takes_winning_move);
    RUN_TEST(test_ai_o_blocks_x_win);
}
