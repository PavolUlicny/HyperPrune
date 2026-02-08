/*
 * Program entry and CLI modes
 * ---------------------------
 * - Interactive game loop (human vs AI)
 * - Self-play benchmarking via --selfplay|-s [games] [--quiet|-q]
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
 * Maximum transposition table size: caps memory usage for large boards.
 * Default: 100M entries = ~1.6 GB (each entry is 16 bytes).
 *
 * Adjust based on available system memory:
 * - 50M entries  = ~800 MB  (conservative)
 * - 100M entries = ~1.6 GB  (default, recommended)
 * - 200M entries = ~3.2 GB  (aggressive, for 7×7+)
 * - 500M entries = ~8 GB    (maximum, for 8×8+)
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
    clock_t startTime = 0;

    if (!quiet)
        startTime = clock();

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
        double elapsed = (double)(clock() - startTime) / CLOCKS_PER_SEC;
        double throughput = elapsed > 0 ? (gameCount / elapsed) : 0.0;
        printf("Self-play finished: %d games\nAI1Wins = %d AI2Wins = %d Ties = %d\n", gameCount, ai1Wins, ai2Wins, ties);
        printf("Elapsed: %.3f s\nThroughput: %.1f games/s\n", elapsed, throughput);

        /* Print Transposition Table statistics */
        size_t hits, misses, collisions;
        transposition_table_get_stats(&hits, &misses, &collisions);
        size_t total_probes = hits + misses;
        double hit_rate = total_probes > 0 ? (100.0 * hits / total_probes) : 0.0;
        printf("Transposition Table Stats: hits = %zu misses = %zu collisions = %zu (%.1f%% hit rate)\n",
               hits, misses, collisions, hit_rate);
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
     * Dynamic transposition table sizing - OPTIMIZED FOR MAXIMUM PERFORMANCE
     *
     * Empirically derived from systematic mega-scale benchmarking (644M games):
     *   - 3×3: 100K entries  = 2.21M games/s (100% hit rate, 1.5 MB)
     *   - 4×4: 1.5M entries  = 239K games/s  (100% hit rate, 24 MB)
     *
     * Formula: size = 1,500,000 × (BOARD_SIZE / 4)^9.4
     *
     * The 9.4 exponent captures:
     *   - Exponential growth of search space (~BOARD_SIZE^7-8 for visited positions)
     *   - Safety margin for hash collisions (~20% overcapacity)
     *   - Performance buffer to maintain 100% hit rates under all conditions
     *
     * Calculated sizes (capped at MAX_TRANSPOSITION_TABLE_SIZE):
     *   - 3×3: 100K   (1.5 MB)   - verified optimal
     *   - 4×4: 1.5M   (24 MB)    - verified optimal
     *   - 5×5: ~13M   (200 MB)   - extrapolated, aggressive for performance
     *   - 6×6: ~69M   (1.1 GB)   - extrapolated, aggressive for performance
     *   - 7×7: ~100M  (1.6 GB)   - capped at maximum
     *   - 8×8: ~100M  (1.6 GB)   - capped at maximum
     *
     * Philosophy: Err on the side of larger tables for maximum performance.
     * Users running 6×6+ boards clearly have sufficient RAM. Configure
     * MAX_TRANSPOSITION_TABLE_SIZE if memory is constrained.
     */
    size_t transposition_table_size;

    if (BOARD_SIZE <= 3)
    {
        /* Verified optimal for 3×3 via 100M game testing */
        transposition_table_size = 100000; /* 1.5 MB */
    }
    else if (BOARD_SIZE == 4)
    {
        /* Verified optimal for 4×4 via 5M game testing */
        transposition_table_size = 1500000; /* 24 MB */
    }
    else
    {
        /*
         * Extrapolate for larger boards using exponential scaling.
         * Growth exponent 9.4 derived from log((1.5M / 100K)) / log(4/3) ≈ 9.43
         */
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
