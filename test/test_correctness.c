#include "unity/unity.h"
#include "../src/MiniMax/mini_max.h"
#include "../src/MiniMax/transposition.h"
#include "../src/TicTacToe/tic_tac_toe.h"
#include <stdlib.h>

// Helper: Play a complete game
int play_full_game(char first_player, uint64_t seed)
{
    zobrist_set_seed(seed);
    zobrist_init();
    transposition_table_init(100000);
    init_win_masks();

    Bitboard board = {0, 0};
    char current = first_player;
    int moves = 0;

    while (moves < MAX_MOVES)
    {
        int row, col;
        getAiMove(board, current, &row, &col);

        if (row == -1)
            break; // Terminal

        bitboard_make_move(&board, row, col, current);
        moves++;

        // Check win
        char opponent = (current == 'x') ? 'o' : 'x';
        uint64_t pieces = (current == 'x') ? board.x_pieces : board.o_pieces;
        if (bitboard_did_last_move_win(pieces, row, col))
        {
            transposition_table_free();
            return (current == 'x') ? 1 : -1; // 1=X wins, -1=O wins
        }

        current = opponent;
    }

    transposition_table_free();
    return 0; // Tie
}

// Test 3x3 optimal play (should always tie)
void test_optimal_play_3x3(void)
{
#if BOARD_SIZE == 3
    int x_wins = 0, o_wins = 0, ties = 0;

    for (int i = 0; i < 100; i++)
    {
        int result = play_full_game('x', i);
        if (result == 1)
            x_wins++;
        else if (result == -1)
            o_wins++;
        else
            ties++;
    }

    TEST_ASSERT_EQUAL(0, x_wins);
    TEST_ASSERT_EQUAL(0, o_wins);
    TEST_ASSERT_EQUAL(100, ties);
#endif
}

// Test determinism
void test_determinism(void)
{
    int result1 = play_full_game('x', 42);
    int result2 = play_full_game('x', 42);

    TEST_ASSERT_EQUAL(result1, result2);
}

void test_correctness_suite(void)
{
    RUN_TEST(test_optimal_play_3x3);
    RUN_TEST(test_determinism);
}
