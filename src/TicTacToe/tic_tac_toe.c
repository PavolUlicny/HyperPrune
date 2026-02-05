/*
 * Tic-Tac-Toe: board state and CLI utilities
 * ------------------------------------------
 *
 * Responsibilities in this file:
 *  - Maintain global board state and player symbols
 *  - Basic I/O helpers for a terminal UI (reading moves, printing board)
 *  - Lightweight result checking after each move
 */

#include <stdlib.h>
#include <stdio.h>
#include "tic_tac_toe.h"

/* Global game state used by the simple CLI program. */
char board[BOARD_SIZE][BOARD_SIZE];
char player_turn = 'x';
static int move_count = 0; /* number of moves played so far */
char human_symbol = 'x';
char ai_symbol = 'o';

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

/* Initialize all board cells to ' ' (empty). */
void initializeBoard(void)
{
    for (int i = 0; i < BOARD_SIZE; i++)
    {
        for (int j = 0; j < BOARD_SIZE; j++)
        {
            board[i][j] = ' ';
        }
    }
}

/* Pretty-print the board with 1-based indices on both axes. */
void printBoard(void)
{
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
            printf("[%*c]", digits, board[i - 1][j - 1]);
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

/* Read and validate a single board coordinate from stdin. Returns 0-based index. */
static int getValidCoord(const char *prompt)
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
                printf("\nEOF received. Exiting game.\n");
                exit(0);
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

        return value - 1;
    }
}

/*
 * Prompt the user for a move as 1-based (column, row), validate input, and
 * return 0-based (row, col). Re-prompts on invalid or out-of-range input.
 * On EOF, exits gracefully.
 */
void getMove(int *out_row, int *out_col)
{
    while (1)
    {
        int col = getValidCoord("Input column: ");
        int row = getValidCoord("Input row: ");

        if (board[row][col] != ' ')
        {
            printf("Cell already occupied. Choose another.\n\n");
            continue;
        }

        *out_row = row;
        *out_col = col;
        return;
    }
}

/* Place the current player's symbol and toggle player_turn; increment move_count. */
void makeMove(int row, int col)
{
    board[row][col] = player_turn;
    player_turn = (player_turn == 'x') ? 'o' : 'x';
    move_count++;
}

/*
 * Check whether the last move at (row,col) finished the game.
 * Scans the affected row, column, and diagonals only.
 * Returns PLAYER_WIN/AI_WIN/TIE/CONTINUE.
 */
GameResult checkWinner(int row, int col)
{
    char player = board[row][col];

    int win = 1;
    for (int c = 0; c < BOARD_SIZE; ++c)
    {
        if (board[row][c] != player)
        {
            win = 0;
            break;
        }
    }
    if (win)
        return (player == human_symbol) ? PLAYER_WIN : AI_WIN;

    win = 1;
    for (int r = 0; r < BOARD_SIZE; ++r)
    {
        if (board[r][col] != player)
        {
            win = 0;
            break;
        }
    }
    if (win)
        return (player == human_symbol) ? PLAYER_WIN : AI_WIN;

    if (row == col)
    {
        win = 1;
        for (int i = 0; i < BOARD_SIZE; ++i)
        {
            if (board[i][i] != player)
            {
                win = 0;
                break;
            }
        }
        if (win)
            return (player == human_symbol) ? PLAYER_WIN : AI_WIN;
    }

    if (row + col == BOARD_SIZE - 1)
    {
        win = 1;
        for (int i = 0; i < BOARD_SIZE; ++i)
        {
            if (board[i][BOARD_SIZE - 1 - i] != player)
            {
                win = 0;
                break;
            }
        }
        if (win)
            return (player == human_symbol) ? PLAYER_WIN : AI_WIN;
    }

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
