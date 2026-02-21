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
        int result = play_full_game('x', (uint64_t)i);
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
#elif BOARD_SIZE == 4
    int x_wins = 0, o_wins = 0, ties = 0;

    for (int i = 0; i < 10; i++)
    {
        int result = play_full_game('x', (uint64_t)i);
        if (result == 1)
            x_wins++;
        else if (result == -1)
            o_wins++;
        else
            ties++;
    }

    TEST_ASSERT_EQUAL(0, x_wins);
    TEST_ASSERT_EQUAL(0, o_wins);
    TEST_ASSERT_EQUAL(10, ties);
#endif
}

// Test determinism
void test_determinism(void)
{
    int result1 = play_full_game('x', UINT64_C(42));
    int result2 = play_full_game('x', UINT64_C(42));

    TEST_ASSERT_EQUAL(result1, result2);
}

// Test 3x3 optimal play with O going first (complements test_optimal_play_3x3 which uses X first)
void test_optimal_play_o_first(void)
{
#if BOARD_SIZE == 3
    int x_wins = 0, o_wins = 0, ties = 0;

    for (int i = 0; i < 100; i++)
    {
        int result = play_full_game('o', (uint64_t)i);
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
#elif BOARD_SIZE == 4
    int x_wins = 0, o_wins = 0, ties = 0;

    for (int i = 0; i < 10; i++)
    {
        int result = play_full_game('o', (uint64_t)i);
        if (result == 1)
            x_wins++;
        else if (result == -1)
            o_wins++;
        else
            ties++;
    }

    TEST_ASSERT_EQUAL(0, x_wins);
    TEST_ASSERT_EQUAL(0, o_wins);
    TEST_ASSERT_EQUAL(10, ties);
#endif
}

// Regression test for the Zobrist side-to-move (turn key) bug.
// Plays games back-to-back WITHOUT reinitialising the TT between them, alternating
// which player goes first. This forces the TT to be reused across games where the
// same board positions are reached at different search depths (max vs min nodes).
// If zobrist_toggle_turn calls are ever removed, this test will fail.
void test_cross_game_tt_no_reinit(void)
{
#if BOARD_SIZE == 3
    zobrist_set_seed(17);
    zobrist_init();
    transposition_table_init(100000);

    int wins = 0;

    for (int g = 0; g < 20; g++)
    {
        char first = (g % 2 == 0) ? 'x' : 'o';
        Bitboard board = {0, 0};
        char current = first;

        for (int m = 0; m < MAX_MOVES; m++)
        {
            int row, col;
            getAiMove(board, current, &row, &col);
            if (row == -1)
                break;

            bitboard_make_move(&board, row, col, current);
            uint64_t pieces = (current == 'x') ? board.x_pieces : board.o_pieces;
            if (bitboard_did_last_move_win(pieces, row, col))
            {
                wins++;
                break;
            }
            current = (current == 'x') ? 'o' : 'x';
        }
    }

    TEST_ASSERT_EQUAL(0, wins);
    transposition_table_free();
#endif
}

void test_correctness_suite(void)
{
    RUN_TEST(test_optimal_play_3x3);
    RUN_TEST(test_determinism);
    RUN_TEST(test_optimal_play_o_first);
    RUN_TEST(test_cross_game_tt_no_reinit);
}
