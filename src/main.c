/*
 * Program entry and CLI modes
 * ---------------------------
 * - Interactive game loop (human vs AI)
 * - Self-play benchmarking via --selfplay|-s [games] [--quiet|-q]
 *   * Default games: 1000 when omitted
 *   * --quiet/-q suppresses timing output
 */

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "TicTacToe/tic_tac_toe.h"
#include "MiniMax/mini_max.h"

/*
 * Interactive human vs AI loop. Prompts the user to choose a symbol, then
 * alternates between human input and AI selection until the game ends.
 */
static void playGame(void)
{
    while (1)
    {
        restartGame();
        choosePlayerSymbol();
        if (player_turn != ai_symbol)
            printBoard();

        while (1)
        {
            int row, col;

            if (player_turn == human_symbol)
            {
                getMove(&row, &col);
                makeMove(row, col);
                GameResult result = checkWinner(row, col);

                if (result != GAME_CONTINUE)
                {
                    printGameResult(result);
                    break;
                }
            }
            else
            {
                int ai_row, ai_col;
                getAiMove(board, ai_symbol, &ai_row, &ai_col);
                makeMove(ai_row, ai_col);
                printf("AI plays (%d, %d)\n", ai_col + 1, ai_row + 1);
                GameResult result = checkWinner(ai_row, ai_col);

                if (result != GAME_CONTINUE)
                {
                    printGameResult(result);
                    break;
                }
                else
                {
                    printBoard();
                }
            }
        }

        if (!askRestart())
            return;
    }
}

/*
 * Self-play benchmarking: runs gameCount AI vs AI games starting from an empty
 * board, alternating turns. Collects win/tie stats and (optionally) prints
 * timing and throughput.
 *
 * Parameters:
 *  - gameCount: number of games to run
 *  - quiet:     when non-zero, suppress timing output
 */
static int selfPlay(int gameCount, int quiet)
{
    int ai1Wins = 0;
    int ai2Wins = 0;
    int ties = 0;
    struct timespec startTime;
    struct timespec endTime;

    if (!quiet)
        clock_gettime(CLOCK_MONOTONIC, &startTime);

    for (int g = 0; g < gameCount; ++g)
    {
        restartGame();

        while (1)
        {
            int currentRow = -1;
            int currentCol = -1;
            char currentPlayer = player_turn;

            getAiMove(board, currentPlayer, &currentRow, &currentCol);
            makeMove(currentRow, currentCol);
            GameResult result = checkWinner(currentRow, currentCol);

            if (result != GAME_CONTINUE)
            {
                if (result == GAME_TIE)
                    ++ties;
                else if (currentPlayer == 'x')
                    ++ai1Wins;
                else
                    ++ai2Wins;
                break;
            }
        }
    }

    if (!quiet)
    {
        clock_gettime(CLOCK_MONOTONIC, &endTime);
        double elapsed = (double)(endTime.tv_sec - startTime.tv_sec) + (double)(endTime.tv_nsec - startTime.tv_nsec) / 1e9;
        double throughput = elapsed > 0 ? (gameCount / elapsed) : 0.0;
        printf("Self-play finished: %d games. AI1Wins=%d AI2Wins=%d Ties=%d\n", gameCount, ai1Wins, ai2Wins, ties);
        printf("Elapsed: %.3f s, Throughput: %.1f games/s\n", elapsed, throughput);
    }

    return 0;
}

/*
 * CLI:
 *  - Default (no args): interactive human vs AI game
 *  - --selfplay|-s [games] [--quiet|-q]: run AI vs AI for N games (default 1000)
 */
int main(int argc, char **argv)
{
    if (argc >= 2 && (strcmp(argv[1], "--selfplay") == 0 || strcmp(argv[1], "-s") == 0))
    {
        int games = 1000;
        int quiet = 0;
        int flag_start = 2;

        if (argc >= 3)
        {
            char *endp;
            long val = strtol(argv[2], &endp, 10);
            if (endp != argv[2] && *endp == '\0')
            {
                if (val < 1)
                {
                    fprintf(stderr, "Game count must be a positive integer.\n");
                    return 1;
                }
                games = (int)val;
                flag_start = 3;
            }
        }

        for (int i = flag_start; i < argc; ++i)
        {
            if (strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "-q") == 0)
                quiet = 1;
        }
        return selfPlay(games, quiet);
    }

    playGame();
    return 0;
}
