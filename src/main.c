/*
 * Program entry and CLI modes
 * ---------------------------
 * - Interactive game loop (human vs AI)
 * - Self-play mode via --selfplay|-s [games] [--quiet|-q]
 *   * Default games: 1000 when omitted
 *   * --quiet/-q suppresses timing output
 */

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "TicTacToe/tic_tac_toe.h"
#include "MiniMax/mini_max.h"
#include "MiniMax/transposition.h"

/*
 * Maximum transposition table size (entry count).
 * This caps the allocation when BOARD_SIZE is large.
 */
#define MAX_TRANSPOSITION_TABLE_SIZE 100000000

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
                getAiMove(board_state, ai_symbol, &ai_row, &ai_col);

                /* Defensive: getAiMove returns (-1, -1) for terminal positions */
                if (ai_row == -1 || ai_col == -1)
                {
                    fprintf(stderr, "Error: AI returned invalid move (terminal position)\n");
                    break; /* Exit game loop */
                }

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
 * Self-play mode: runs gameCount AI vs AI games starting from an empty
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
    clock_t startTime = 0;
    int timing_available = 0;

    if (!quiet)
    {
        startTime = clock();
        if (startTime == (clock_t)-1)
        {
            fprintf(stderr, "Warning: clock() failed, timing stats will be unavailable\n");
            timing_available = 0;
        }
        else
        {
            timing_available = 1;
        }
    }

    for (int g = 0; g < gameCount; ++g)
    {
        restartGame();

        while (1)
        {
            int currentRow = -1;
            int currentCol = -1;
            char currentPlayer = player_turn;

            getAiMove(board_state, currentPlayer, &currentRow, &currentCol);

            /* Defensive: getAiMove returns (-1, -1) for terminal positions */
            if (currentRow == -1 || currentCol == -1)
            {
                fprintf(stderr, "Error: AI returned invalid move in self-play (game %d)\n", g + 1);
                return 1; /* Exit with error */
            }

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
        double elapsed = 0.0;
        double throughput = 0.0;

        /* Try to get timing data if clock was available at start */
        if (timing_available)
        {
            clock_t endTime = clock();
            if (endTime == (clock_t)-1)
            {
                fprintf(stderr, "Warning: clock() failed at end, timing stats unavailable\n");
                timing_available = 0;
            }
            else
            {
                elapsed = (double)(endTime - startTime) / CLOCKS_PER_SEC;
                if (elapsed < 0)
                {
                    fprintf(stderr, "Warning: negative elapsed time, timing stats unavailable\n");
                    timing_available = 0;
                }
                else
                {
                    throughput = elapsed > 0 ? (gameCount / elapsed) : 0.0;
                }
            }
        }

        /* Calculate percentages */
        double ai1_pct = (100.0 * ai1Wins) / gameCount;
        double ai2_pct = (100.0 * ai2Wins) / gameCount;
        double tie_pct = (100.0 * ties) / gameCount;

        /* Print results header */
        printf("\n");
        printf("═══════════════════════════════════════════════════════════════\n");
        printf("  Self-Play Results: %d games\n", gameCount);
        printf("═══════════════════════════════════════════════════════════════\n");

        /* Print game outcomes (always available) */
        printf("  Outcomes\n");
        printf("    X wins:  %8d  (%5.1f%%)\n", ai1Wins, ai1_pct);
        printf("    O wins:  %8d  (%5.1f%%)\n", ai2Wins, ai2_pct);
        printf("    Ties:    %8d  (%5.1f%%)\n", ties, tie_pct);
        printf("\n");

        /* Print performance stats (only if timing available) */
        if (timing_available)
        {
            printf("  Performance\n");
            printf("    Elapsed:     %8.3f s\n", elapsed);
            if (throughput >= 1000000.0)
                printf("    Throughput:  %8.2f M games/s\n", throughput / 1000000.0);
            else if (throughput >= 1000.0)
                printf("    Throughput:  %8.2f K games/s\n", throughput / 1000.0);
            else
                printf("    Throughput:  %8.1f games/s\n", throughput);
            printf("\n");
        }

        /* Print Transposition Table statistics */
        size_t hits, misses, collisions;
        transposition_table_get_stats(&hits, &misses, &collisions);

        /* Check for overflow in total_probes calculation */
        size_t total_probes = hits + misses + collisions;
        double hit_rate = 0.0;
        double miss_rate = 0.0;
        double collision_rate = 0.0;

        if (total_probes < hits || total_probes < misses || total_probes < collisions)
        {
            /* Overflow detected - calculate rates using double to avoid wrap */
            double total_probes_d = (double)hits + (double)misses + (double)collisions;
            hit_rate = (100.0 * hits) / total_probes_d;
            miss_rate = (100.0 * misses) / total_probes_d;
            collision_rate = (100.0 * collisions) / total_probes_d;
        }
        else if (total_probes > 0)
        {
            hit_rate = (100.0 * hits) / total_probes;
            miss_rate = (100.0 * misses) / total_probes;
            collision_rate = (100.0 * collisions) / total_probes;
        }

        printf("  Transposition Table\n");
        printf("    Hits:        %12zu  (%5.1f%%)\n", hits, hit_rate);
        printf("    Misses:      %12zu  (%5.1f%%)\n", misses, miss_rate);
        printf("    Collisions:  %12zu  (%5.1f%%)\n", collisions, collision_rate);
        printf("═══════════════════════════════════════════════════════════════\n");
        printf("\n");
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
    /* Initialize win masks for bitboard operations */
    init_win_masks();

    /* Initialize Zobrist hashing and transposition table */
    zobrist_init();

    /*
     * Dynamic transposition table sizing.
     * Formula: size = 1,500,000 × (BOARD_SIZE / 4)^9.4
     * The result is capped at MAX_TRANSPOSITION_TABLE_SIZE.
     */
    size_t transposition_table_size;

    if (BOARD_SIZE <= 3)
    {
        transposition_table_size = 100000;
    }
    else if (BOARD_SIZE == 4)
    {
        transposition_table_size = 1500000;
    }
    else
    {
        /* Extrapolate for larger boards using exponential scaling. */
        double growth_factor = pow((double)BOARD_SIZE / 4.0, 9.4);
        transposition_table_size = (size_t)(1500000.0 * growth_factor);

        /* Apply maximum cap to prevent excessive memory usage */
        if (transposition_table_size > MAX_TRANSPOSITION_TABLE_SIZE)
        {
            transposition_table_size = MAX_TRANSPOSITION_TABLE_SIZE;
        }
    }

    transposition_table_init(transposition_table_size);

    int ret_code = 0;

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
                if (val < 1 || val > INT_MAX)
                {
                    fprintf(stderr, "Game count must be a positive integer.\n");
                    transposition_table_free();
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
        ret_code = selfPlay(games, quiet);
    }
    else
    {
        playGame();
    }

    /* Clean up transposition table */
    transposition_table_free();
    return ret_code;
}
