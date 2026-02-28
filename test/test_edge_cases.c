#include "unity/unity.h"
#include "../src/TicTacToe/tic_tac_toe.h"
#include "../src/Negamax/negamax.h"
#include "../src/Negamax/transposition.h"

// Test bitboard_did_last_move_win for row win
void test_did_last_move_win_row(void)
{
    Bitboard board = {0, 0};

    // Create row win at row 0
    for (int c = 0; c < BOARD_SIZE; c++)
    {
        bitboard_make_move_rc(&board, 0, c, 'x');
    }

    // Check that last move at (0, BOARD_SIZE-1) wins
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.x_pieces, 0, BOARD_SIZE - 1));

    // Check that same config for a different cell also detects win
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.x_pieces, 0, 0));
}

// Test bitboard_did_last_move_win for column win
void test_did_last_move_win_col(void)
{
    Bitboard board = {0, 0};

    // Create column win at col 0
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        bitboard_make_move_rc(&board, r, 0, 'o');
    }

    // Check that last move at (BOARD_SIZE-1, 0) wins
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.o_pieces, BOARD_SIZE - 1, 0));
}

// Test bitboard_did_last_move_win for main diagonal
void test_did_last_move_win_main_diagonal(void)
{
    Bitboard board = {0, 0};

    // Create main diagonal win
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        bitboard_make_move_rc(&board, i, i, 'x');
    }

    // Check center of diagonal
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.x_pieces, BOARD_SIZE / 2, BOARD_SIZE / 2));
}

// Test bitboard_did_last_move_win for anti-diagonal (comprehensive - all positions)
void test_did_last_move_win_anti_diagonal(void)
{
    Bitboard board = {0, 0};

    for (int i = 0; i < BOARD_SIZE; i++)
        bitboard_make_move_rc(&board, i, BOARD_SIZE - 1 - i, 'x');

    for (int i = 0; i < BOARD_SIZE; i++)
        TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.x_pieces, i, BOARD_SIZE - 1 - i));
}

// Test bitboard_did_last_move_win returns false for non-winning board
void test_did_last_move_win_no_win(void)
{
    Bitboard board = {0, 0};

    // Partial row (not a win)
    bitboard_make_move_rc(&board, 0, 0, 'x');
    bitboard_make_move_rc(&board, 0, 1, 'x');

    TEST_ASSERT_FALSE(bitboard_did_last_move_win(board.x_pieces, 0, 1));
}

// Test bitboard_has_won returns false for empty board
void test_has_won_empty_board(void)
{
    Bitboard board = {0, 0};

    TEST_ASSERT_FALSE(bitboard_has_won(board.x_pieces));
    TEST_ASSERT_FALSE(bitboard_has_won(board.o_pieces));
}

// Test bitboard_has_won returns false for partial board
void test_has_won_partial_board(void)
{
    Bitboard board = {0, 0};

    bitboard_make_move_rc(&board, 0, 0, 'x');
    bitboard_make_move_rc(&board, 1, 1, 'x');
    bitboard_make_move_rc(&board, 0, 1, 'o');

    TEST_ASSERT_FALSE(bitboard_has_won(board.x_pieces));
    TEST_ASSERT_FALSE(bitboard_has_won(board.o_pieces));
}

// Test multiple make moves without unmake
void test_multiple_makes(void)
{
    Bitboard board = {0, 0};

    // Make multiple moves
    bitboard_make_move_rc(&board, 0, 0, 'x');
    bitboard_make_move_rc(&board, 1, 1, 'o');
    bitboard_make_move_rc(&board, 2, 2, 'x');

    // Verify all are set
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, 0, 0));
    TEST_ASSERT_EQUAL('o', bitboard_get_cell(board, 1, 1));
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, 2, 2));
}

// Test unmake without prior make (should result in no change)
void test_unmake_empty_cell(void)
{
    Bitboard board = {0, 0};
    bitboard_make_move_rc(&board, 0, 0, 'x');

    // Unmake a cell that was never set
    bitboard_unmake_move_rc(&board, 1, 1, 'o');

    // Board should be unchanged except for (0,0)
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, 0, 0));
    TEST_ASSERT_EQUAL(' ', bitboard_get_cell(board, 1, 1));
}

// Test both players having pieces
void test_both_players_pieces(void)
{
    Bitboard board = {0, 0};

    bitboard_make_move_rc(&board, 0, 0, 'x');
    bitboard_make_move_rc(&board, 0, 1, 'x');
    bitboard_make_move_rc(&board, 1, 0, 'o');
    bitboard_make_move_rc(&board, 1, 1, 'o');

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
    bitboard_make_move_rc(&board, 0, 0, 'x');
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, 0, 0));

    // Top-right
    bitboard_make_move_rc(&board, 0, BOARD_SIZE - 1, 'o');
    TEST_ASSERT_EQUAL('o', bitboard_get_cell(board, 0, BOARD_SIZE - 1));

    // Bottom-left
    bitboard_make_move_rc(&board, BOARD_SIZE - 1, 0, 'x');
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board, BOARD_SIZE - 1, 0));

    // Bottom-right
    bitboard_make_move_rc(&board, BOARD_SIZE - 1, BOARD_SIZE - 1, 'o');
    TEST_ASSERT_EQUAL('o', bitboard_get_cell(board, BOARD_SIZE - 1, BOARD_SIZE - 1));
}

// Test make/unmake cycle preserves hash
void test_make_unmake_hash_cycle(void)
{
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    uint64_t original_hash = zobrist_hash(board);

    // Make move and hash
    bitboard_make_move_rc(&board, 1, 1, 'x');
    uint64_t new_hash = zobrist_toggle(original_hash, POS_TO_BIT(1, 1), 'x');

    // Unmake and restore hash
    bitboard_unmake_move_rc(&board, 1, 1, 'x');
    uint64_t restored_hash = zobrist_toggle(new_hash, POS_TO_BIT(1, 1), 'x');

    TEST_ASSERT_EQUAL_UINT64(original_hash, restored_hash);
}

// Test zobrist hash is non-zero for a non-empty board
void test_zobrist_hash_nonzero(void)
{
    zobrist_set_seed(42);
    zobrist_init();

    Bitboard board = {0, 0};
    bitboard_make_move_rc(&board, 0, 0, 'x'); /* Non-empty board: piece key is non-zero */
    uint64_t hash = zobrist_hash(board);

    // (Statistical property - not guaranteed but extremely likely)
    TEST_ASSERT_NOT_EQUAL_UINT64(0, hash);
}

// Test overlapping bitboard pieces (invalid state)
void test_overlapping_bitboard_pieces(void)
{
    Bitboard board = {0, 0};

    // Manually create invalid state (overlap at 0,0)
    board.x_pieces = BIT_MASK(0, 0);
    board.o_pieces = BIT_MASK(0, 0);

    // bitboard_get_cell returns 'x' first (documents priority behavior)
    char cell = bitboard_get_cell(board, 0, 0);
    TEST_ASSERT_EQUAL('x', cell);
}

// Test win detection called on empty cell
void test_did_last_move_win_empty_cell(void)
{
    Bitboard board = {0, 0};

    // Call on empty cell (no piece there)
    int result = bitboard_did_last_move_win(board.x_pieces, 0, 0);

    // Should return false (no pieces to form line)
    TEST_ASSERT_FALSE(result);
}

// Test both players won (invalid state - documents priority)
void test_both_players_won_invalid_state(void)
{
    Bitboard board = {0, 0};

    // X fills row 0, O fills row 1 — impossible game state where both have won
    for (int c = 0; c < BOARD_SIZE; c++)
    {
        bitboard_make_move_rc(&board, 0, c, 'x');
        bitboard_make_move_rc(&board, 1, c, 'o');
    }

    TEST_ASSERT_TRUE(bitboard_has_won(board.x_pieces));
    TEST_ASSERT_TRUE(bitboard_has_won(board.o_pieces));
}

// Test bitboard_did_last_move_win for the last column (BOARD_SIZE-1).
// The existing test_did_last_move_win_col only checks column 0.
void test_did_last_move_win_col_last_column(void)
{
    Bitboard board = {0, 0};

    for (int r = 0; r < BOARD_SIZE; r++)
        bitboard_make_move_rc(&board, r, BOARD_SIZE - 1, 'x');

    // Both ends of the column should report a win
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.x_pieces, 0, BOARD_SIZE - 1));
    TEST_ASSERT_TRUE(bitboard_did_last_move_win(board.x_pieces, BOARD_SIZE - 1, BOARD_SIZE - 1));
}

// Test checkWinner returns X_WIN when X completes a row
void test_checkWinner_x_win(void)
{
    restartGame();
    for (int c = 0; c < BOARD_SIZE; c++)
        bitboard_make_move_rc(&board_state, 0, c, 'x');

    TEST_ASSERT_EQUAL(X_WIN, checkWinner(0, BOARD_SIZE - 1));
}

// Test checkWinner returns O_WIN when O completes a column
void test_checkWinner_o_win(void)
{
    restartGame();
    for (int r = 0; r < BOARD_SIZE; r++)
        bitboard_make_move_rc(&board_state, r, 0, 'o');

    TEST_ASSERT_EQUAL(O_WIN, checkWinner(BOARD_SIZE - 1, 0));
}

// Test checkWinner returns GAME_CONTINUE on a non-terminal board
void test_checkWinner_continue(void)
{
    restartGame();
    bitboard_make_move_rc(&board_state, 0, 0, 'x');

    TEST_ASSERT_EQUAL(GAME_CONTINUE, checkWinner(0, 0));
}

// Test checkWinner returns GAME_TIE when the board is full with no winner.
// Uses makeMove so move_count reaches MAX_MOVES.
// 3x3 produces: X O X / X O X / O X O — verified no winner.
// 4x4 produces: X X O O / O O X X / X X O O / O O X X — verified no winner.
void test_checkWinner_tie(void)
{
#if BOARD_SIZE == 3
    restartGame();
    int moves[][2] = {{0, 0}, {0, 1}, {0, 2}, {1, 1}, {1, 0}, {2, 0}, {1, 2}, {2, 2}, {2, 1}};
    for (int i = 0; i < 9; i++)
        makeMove(moves[i][0], moves[i][1]);
    TEST_ASSERT_EQUAL(GAME_TIE, checkWinner(2, 1));
#elif BOARD_SIZE == 4
    restartGame();
    int moves[][2] = {
        {0, 0}, {0, 2}, {0, 1}, {0, 3}, {1, 2}, {1, 0}, {1, 3}, {1, 1}, {2, 0}, {2, 2}, {2, 1}, {2, 3}, {3, 2}, {3, 0}, {3, 3}, {3, 1}};
    for (int i = 0; i < 16; i++)
        makeMove(moves[i][0], moves[i][1]);
    TEST_ASSERT_EQUAL(GAME_TIE, checkWinner(3, 1));
#endif
}

// Test makeMove places the current player's piece and flips player_turn
void test_makeMove_places_piece_and_flips_turn(void)
{
    restartGame();

    makeMove(0, 0);
    TEST_ASSERT_EQUAL('x', bitboard_get_cell(board_state, 0, 0));
    TEST_ASSERT_EQUAL('o', player_turn);

    makeMove(1, 1);
    TEST_ASSERT_EQUAL('o', bitboard_get_cell(board_state, 1, 1));
    TEST_ASSERT_EQUAL('x', player_turn);
}

// Test restartGame clears the board, resets player_turn to 'x', and resets move_count
void test_restartGame_clears_board_and_resets_turn(void)
{
    restartGame();
    makeMove(0, 0);
    makeMove(1, 1);

    restartGame();

    TEST_ASSERT_EQUAL('x', player_turn);

    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            TEST_ASSERT_EQUAL(' ', bitboard_get_cell(board_state, r, c));

    /*
     * Verify move_count was zeroed by playing a full tie-board sequence via makeMove()
     * and asserting GAME_CONTINUE on the penultimate move and GAME_TIE on the last.
     *
     * Fill pattern: player(r,c) = ((c/2 + r) % 2 == 0) ? 'x' : 'o'
     * This places pieces in 2-column alternating bands. Every row, column, and diagonal
     * in the resulting board contains both 'x' and 'o' pieces, so no line is ever
     * completed by one player — making it a win-free tie for all board sizes 3-8.
     *
     * Since every line in the final board is mixed, no partial prefix of the fill can
     * complete a line for one player either, so no spurious win fires mid-sequence.
     *
     * If restartGame() failed to reset move_count (left it at 2), move_count would
     * reach MAX_MOVES after only MAX_MOVES-2 post-restart calls, and checkWinner()
     * would return GAME_TIE instead of GAME_CONTINUE on the penultimate move.
     */
    int x_bits[MAX_MOVES], o_bits[MAX_MOVES];
    int nx = 0, no_count = 0;
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            if (((c / 2) + r) % 2 == 0)
                x_bits[nx++] = r * BOARD_SIZE + c;
            else
                o_bits[no_count++] = r * BOARD_SIZE + c;
        }

    /* Interleave x and o positions; makeMove() alternates turns automatically */
    int xi = 0, oi = 0;
    int last_row = 0, last_col = 0;
    for (int i = 0; i < MAX_MOVES - 1; i++)
    {
        int bit = (i % 2 == 0) ? x_bits[xi++] : o_bits[oi++];
        last_row = bit / BOARD_SIZE;
        last_col = bit % BOARD_SIZE;
        makeMove(last_row, last_col);
    }
    TEST_ASSERT_EQUAL(GAME_CONTINUE, checkWinner(last_row, last_col));

    /* Final move: completes the board, must be GAME_TIE (not a win) */
    int final_bit = (xi < nx) ? x_bits[xi] : o_bits[oi];
    makeMove(final_bit / BOARD_SIZE, final_bit % BOARD_SIZE);
    TEST_ASSERT_EQUAL(GAME_TIE, checkWinner(final_bit / BOARD_SIZE, final_bit % BOARD_SIZE));
}

// Test bitboard_has_won returns false for a completely full board with no winner.
// test_tie_scenario implicitly relies on this, but the full-board OR-check in
// getAiMove fires first, so a false positive from bitboard_has_won would not be
// caught there. This test checks bitboard_has_won directly on a full tie board.
//
// Board layout: 2-column alternating bands — every row, column, and diagonal
// contains both 'x' and 'o' pieces, so no line is complete for either player.
// The pattern is: player(r,c) = (((c/2) + r) % 2 == 0) ? 'x' : 'o'
void test_has_won_full_board_no_winner(void)
{
    Bitboard board = {0, 0};
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
        {
            char player = (((c / 2) + r) % 2 == 0) ? 'x' : 'o';
            bitboard_make_move_rc(&board, r, c, player);
        }

    TEST_ASSERT_FALSE(bitboard_has_won(board.x_pieces));
    TEST_ASSERT_FALSE(bitboard_has_won(board.o_pieces));
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
    RUN_TEST(test_has_won_full_board_no_winner);
    RUN_TEST(test_multiple_makes);
    RUN_TEST(test_unmake_empty_cell);
    RUN_TEST(test_both_players_pieces);
    RUN_TEST(test_all_cells_empty);
    RUN_TEST(test_corner_cells);
    RUN_TEST(test_make_unmake_hash_cycle);
    RUN_TEST(test_zobrist_hash_nonzero);
    RUN_TEST(test_overlapping_bitboard_pieces);
    RUN_TEST(test_did_last_move_win_empty_cell);
    RUN_TEST(test_both_players_won_invalid_state);
    RUN_TEST(test_did_last_move_win_col_last_column);
    RUN_TEST(test_checkWinner_x_win);
    RUN_TEST(test_checkWinner_o_win);
    RUN_TEST(test_checkWinner_continue);
    RUN_TEST(test_checkWinner_tie);
    RUN_TEST(test_makeMove_places_piece_and_flips_turn);
    RUN_TEST(test_restartGame_clears_board_and_resets_turn);
}
