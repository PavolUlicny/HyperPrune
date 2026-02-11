/*
 * Tic-Tac-Toe: board state and CLI utilities
 * ------------------------------------------
 *
 * Responsibilities in this file:
 *  - Maintain global board state and player symbols
 *  - Basic I/O helpers for a terminal UI (reading moves, printing board)
 *  - Result checking after each move
 */

#include <stdlib.h>
#include <stdio.h>
#include "tic_tac_toe.h"

/* Global game state used by the simple CLI program. */
Bitboard board_state = {0, 0};
char player_turn = 'x';
static int move_count = 0; /* number of moves played so far */
char human_symbol = 'x';
char ai_symbol = 'o';

/* Win detection masks for rows, columns, and diagonals */
static uint64_t win_masks[2 * BOARD_SIZE + 2];
static int win_mask_count = 0;

/* Consume the rest of the current input line (including newline). */
static void discardLine(void)
{
    int ch;

    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}

/* Count how many digits are needed to print a non-negative integer. */
static int numDigits(int number)
{
    int digitQuantity = 1;
    while (number >= 10)
    {
        number /= 10;
        ++digitQuantity;
    }
    return digitQuantity;
}

/* Initialize win masks for bitboard win detection */
void init_win_masks(void)
{
    int idx = 0;

    /* Row masks */
    for (int r = 0; r < BOARD_SIZE; r++)
    {
        uint64_t mask = 0;
        for (int c = 0; c < BOARD_SIZE; c++)
            mask |= BIT_MASK(r, c);
        win_masks[idx++] = mask;
    }

    /* Column masks */
    for (int c = 0; c < BOARD_SIZE; c++)
    {
        uint64_t mask = 0;
        for (int r = 0; r < BOARD_SIZE; r++)
            mask |= BIT_MASK(r, c);
        win_masks[idx++] = mask;
    }

    /* Main diagonal */
    uint64_t mask = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
        mask |= BIT_MASK(i, i);
    win_masks[idx++] = mask;

    /* Anti-diagonal */
    mask = 0;
    for (int i = 0; i < BOARD_SIZE; i++)
        mask |= BIT_MASK(i, BOARD_SIZE - 1 - i);
    win_masks[idx++] = mask;

    win_mask_count = idx;
}

/* Check if a player has won using pre-computed masks */
int bitboard_has_won(uint64_t player_pieces)
{
    for (int i = 0; i < win_mask_count; i++)
    {
        if ((player_pieces & win_masks[i]) == win_masks[i])
            return 1;
    }
    return 0;
}

/* Win check based on last move */
int bitboard_did_last_move_win(uint64_t player_pieces, int row, int col)
{
    /* Check row */
    if ((player_pieces & win_masks[row]) == win_masks[row])
        return 1;

    /* Check column */
    if ((player_pieces & win_masks[BOARD_SIZE + col]) == win_masks[BOARD_SIZE + col])
        return 1;

    /* Check main diagonal (if on it) */
    if (row == col && (player_pieces & win_masks[2 * BOARD_SIZE]) == win_masks[2 * BOARD_SIZE])
        return 1;

    /* Check anti-diagonal (if on it) */
    if (row + col == BOARD_SIZE - 1 &&
        (player_pieces & win_masks[2 * BOARD_SIZE + 1]) == win_masks[2 * BOARD_SIZE + 1])
        return 1;

    return 0;
}

/* Convert bitboard to char array for display */
static void bitboard_to_array(Bitboard board, char out_array[BOARD_SIZE][BOARD_SIZE])
{
    for (int r = 0; r < BOARD_SIZE; r++)
        for (int c = 0; c < BOARD_SIZE; c++)
            out_array[r][c] = bitboard_get_cell(board, r, c);
}

/* Initialize all board cells to ' ' (empty). */
void initializeBoard(void)
{
    board_state.x_pieces = 0;
    board_state.o_pieces = 0;
}

/* Pretty-print the board with 1-based indices on both axes. */
void printBoard(void)
{
    char display[BOARD_SIZE][BOARD_SIZE];
    bitboard_to_array(board_state, display);

    int digits = numDigits(BOARD_SIZE);

    putchar('\n');

    printf("%*s", digits, "");
    for (int c = 1; c <= BOARD_SIZE; ++c)
    {
        printf(" %*d ", digits, c);
    }
    putchar('\n');

    for (int i = 1; i <= BOARD_SIZE; ++i)
    {
        printf("%*d", digits, i);

        for (int j = 1; j <= BOARD_SIZE; ++j)
        {
            printf("[%*c]", digits, display[i - 1][j - 1]);
        }
        putchar('\n');
    }
    putchar('\n');
}

/* Print a message and the final board for a terminal result. */
void printGameResult(GameResult result)
{
    switch (result)
    {
    case PLAYER_WIN:
        printf("Player wins!\n");
        printBoard();
        printf("\n");
        break;
    case AI_WIN:
        printf("AI wins!\n");
        printBoard();
        printf("\n");
        break;
    case GAME_TIE:
        printf("It's a tie!\n");
        printBoard();
        printf("\n");
        break;
    case GAME_CONTINUE:
    default:
        printf("Game continues...\n");
        break;
    }
}

/*
 * Read and validate a single board coordinate from stdin.
 * Returns:
 *   0 on success (coordinate stored in *out_value as 0-based index)
 *  -1 on EOF (user wants to quit)
 */
static int getValidCoord(const char *prompt, int *out_value)
{
    int value;

    while (1)
    {
        printf("%s", prompt);
        int rc = scanf("%d", &value);

        if (rc != 1)
        {
            if (rc == EOF)
            {
                return -1; /* Signal EOF to caller */
            }

            printf("Invalid input. Enter a number 1-%d.\n", BOARD_SIZE);
            discardLine();
            continue;
        }

        discardLine();

        if (value < 1 || value > BOARD_SIZE)
        {
            printf("Out of range (1-%d).\n", BOARD_SIZE);
            continue;
        }

        *out_value = value - 1;
        return 0; /* Success */
    }
}

/*
 * Prompt the user for a move as 1-based (column, row), validate input, and
 * return 0-based (row, col). Re-prompts on invalid or out-of-range input.
 * Returns:
 *   0 on success (move stored in *out_row, *out_col)
 *  -1 on EOF (user pressed Ctrl+D to quit)
 */
int getMove(int *out_row, int *out_col)
{
    while (1)
    {
        int col, row;

        if (getValidCoord("Input column: ", &col) == -1)
        {
            return -1; /* EOF received */
        }

        if (getValidCoord("Input row: ", &row) == -1)
        {
            return -1; /* EOF received */
        }

        if (!bitboard_is_empty(board_state, row, col))
        {
            printf("Cell already occupied. Choose another.\n\n");
            continue;
        }

        *out_row = row;
        *out_col = col;
        return 0; /* Success */
    }
}

/* Place the current player's symbol and toggle player_turn; increment move_count. */
void makeMove(int row, int col)
{
    bitboard_make_move(&board_state, row, col, player_turn);
    player_turn = (player_turn == 'x') ? 'o' : 'x';
    move_count++;
}

/*
 * Check whether the last move at (row,col) finished the game.
 * Uses bitboard win detection for checking.
 * Returns PLAYER_WIN/AI_WIN/TIE/CONTINUE.
 */
GameResult checkWinner(int row, int col)
{
    char player = bitboard_get_cell(board_state, row, col);
    uint64_t player_pieces = (player == 'x') ? board_state.x_pieces : board_state.o_pieces;

    if (bitboard_did_last_move_win(player_pieces, row, col))
        return (player == human_symbol) ? PLAYER_WIN : AI_WIN;

    if (move_count < MAX_MOVES)
        return GAME_CONTINUE;

    return GAME_TIE;
}

/* Reset the board and counters to start a fresh game. */
void restartGame(void)
{
    initializeBoard();
    move_count = 0;
    player_turn = 'x';
}

/* Ask user to restart; returns 1 for yes, 0 for no. */
int askRestart(void)
{
    while (1)
    {
        printf("Play again? (y/n): ");
        fflush(stdout);
        int ch = getchar();
        if (ch == EOF)
        {
            printf("\n");
            return 0;
        }
        if (ch == '\n')
            continue;
        int answer = ch;
        discardLine();
        if (answer == 'y' || answer == 'Y')
        {
            putchar('\n');
            return 1;
        }
        if (answer == 'n' || answer == 'N')
        {
            putchar('\n');
            return 0;
        }
        printf("Please enter y or n.\n");
    }
}

/* Let the user choose 'x' or 'o'; defaults to 'x' on EOF. */
void choosePlayerSymbol(void)
{
    while (1)
    {
        printf("Choose your symbol (x/o): ");
        fflush(stdout);
        int ch = getchar();

        if (ch == EOF)
        {
            printf("\nEOF received. Defaulting to x.\n");
            human_symbol = 'x';
            ai_symbol = 'o';
            discardLine();
            break;
        }

        if (ch == '\n')
            continue;

        int choice = ch;
        discardLine();

        if (choice == 'x' || choice == 'X')
        {
            human_symbol = 'x';
            ai_symbol = 'o';
            break;
        }
        else if (choice == 'o' || choice == 'O')
        {
            human_symbol = 'o';
            ai_symbol = 'x';
            break;
        }
        else
        {
            printf("Please enter x or o.\n");
        }
    }
}
