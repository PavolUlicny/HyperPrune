/*
 * Program entry and CLI modes
 * ---------------------------
 * - Interactive game loop (human vs AI)
 * - Self-play mode via --selfplay|-s [games] [--quiet|-q] [--tt-size|-t SIZE] [--seed SEED]
 *   * Default games: 1000 when omitted
 *   * --quiet/-q suppresses all self-play output
 *   * --tt-size/-t overrides transposition table size
 *   * --seed sets PRNG seed for Zobrist keys (deterministic by default)
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include "TicTacToe/tic_tac_toe.h"
#include "MiniMax/mini_max.h"
#include "MiniMax/transposition.h"

/*
 * Maximum transposition table size (entry count).
 * This caps the allocation when BOARD_SIZE is large.
 * At 16 bytes per entry: 250M entries = 4 GB memory
 */
#define MAX_TRANSPOSITION_TABLE_SIZE 250000000

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
                if (getMove(&row, &col) == -1)
                {
                    printf("\nEOF received. Exiting game.\n");
                    return; /* Clean exit on EOF */
                }

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
 *  - quiet:     when non-zero, suppress all self-play output
 */
static int selfPlay(int gameCount, int quiet)
{
    int ai1Wins = 0;
    int ai2Wins = 0;
    int ties = 0;
    struct timespec startTime = {0};
    int timing_available = 0;

    if (!quiet)
    {
        if (clock_gettime(CLOCK_MONOTONIC, &startTime) != 0)
        {
            fprintf(stderr, "Warning: clock_gettime() failed, timing stats will be unavailable\n");
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
            struct timespec endTime;
            if (clock_gettime(CLOCK_MONOTONIC, &endTime) != 0)
            {
                fprintf(stderr, "Warning: clock_gettime() failed at end, timing stats unavailable\n");
                timing_available = 0;
            }
            else
            {
                /* Calculate elapsed time in seconds */
                elapsed = (endTime.tv_sec - startTime.tv_sec) +
                          (endTime.tv_nsec - startTime.tv_nsec) / 1e9;

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
        printf("===============================================================\n");
        printf("  Self-Play Results: %d games\n", gameCount);
        printf("===============================================================\n");

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
        printf("===============================================================\n");
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
    /* Check for help flag first (before any initialization) */
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            printf("Usage: ttt [OPTIONS]\n\n");
            printf("Play Tic-Tac-Toe against a perfect minimax AI or run self-play simulations.\n\n");
            printf("OPTIONS:\n");
            printf("  Interactive Mode (default):\n");
            printf("    Start an interactive game against the AI.\n\n");
            printf("  Self-Play Mode:\n");
            printf("    --selfplay, -s [GAMES]    Run self-play simulations (default: 1000 games)\n");
            printf("    --quiet, -q               Suppress output\n\n");
            printf("  Configuration:\n");
            printf("    --tt-size SIZE, -t SIZE   Transposition table size in entries\n");
            printf("                              (default: auto-sized, max: %d)\n", MAX_TRANSPOSITION_TABLE_SIZE);
            printf("    --seed SEED               PRNG seed for Zobrist keys (default: deterministic)\n\n");
            printf("  Help:\n");
            printf("    --help, -h                Show this help message and exit\n\n");
            printf("EXAMPLES:\n");
            printf("  ttt                          # Interactive game\n");
            printf("  ttt --selfplay 5000          # Run 5000 self-play games\n");
            printf("  ttt --selfplay 10000 -q      # Run 10000 games, quiet output\n");
            printf("  ttt --seed 42 -s 1000        # Deterministic game with seed 42\n");
            return 0;
        }
    }

    /* Initialize win masks for bitboard operations */
    init_win_masks();

    /* Early parse for --seed flag to allow seeding Zobrist key generation */
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--seed") == 0)
        {
            if (i + 1 < argc)
            {
                const char *seed_str = argv[i + 1];

                /* Check for empty string or another flag */
                if (seed_str[0] == '\0' || seed_str[0] == '-')
                {
                    fprintf(stderr, "Error: --seed requires a value (0 to %llu)\n",
                            (unsigned long long)ULLONG_MAX);
                    exit(EXIT_FAILURE);
                }

                char *endptr;
                errno = 0; /* Must reset errno before strtoull */
                unsigned long long val = strtoull(seed_str, &endptr, 10);

                /* Check for parsing errors */
                if (endptr == seed_str || *endptr != '\0')
                {
                    /* No conversion occurred or trailing garbage */
                    fprintf(stderr, "Error: Invalid --seed value '%s' (not a valid number)\n", seed_str);
                    exit(EXIT_FAILURE);
                }
                else if (errno == ERANGE)
                {
                    fprintf(stderr, "Error: --seed value '%s' out of range (max: %llu)\n",
                            seed_str, (unsigned long long)ULLONG_MAX);
                    exit(EXIT_FAILURE);
                }
                else
                {
                    zobrist_set_seed((uint64_t)val);
                }
            }
            else
            {
                fprintf(stderr, "Error: --seed requires a value\n");
                exit(EXIT_FAILURE);
            }
            break;
        }
    }

    /* Initialize Zobrist hashing and transposition table */
    zobrist_init();

    /* Early parse for --tt-size flag to allow overriding transposition table size */
    int tt_size_override = -1;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--tt-size") == 0 || strcmp(argv[i], "-t") == 0)
        {
            if (i + 1 < argc)
            {
                char *endptr;
                long val = strtol(argv[i + 1], &endptr, 10);
                /* Check if it's a valid number (not just a flag like --quiet) */
                if (*endptr == '\0')
                {
                    /* It's a number, validate range */
                    if (val > 0 && val <= MAX_TRANSPOSITION_TABLE_SIZE)
                    {
                        tt_size_override = (int)val;
                    }
                    else
                    {
                        fprintf(stderr, "Warning: Invalid --tt-size value '%s', must be 1-%d\n", argv[i + 1], MAX_TRANSPOSITION_TABLE_SIZE);
                    }
                }
                else if (argv[i + 1][0] == '-' && !isdigit((unsigned char)argv[i + 1][1]))
                {
                    /* Starts with - but not a negative number, probably a flag */
                    fprintf(stderr, "Warning: --tt-size requires a value\n");
                }
                else
                {
                    /* Not a valid number */
                    fprintf(stderr, "Warning: Invalid --tt-size value '%s', must be 1-%d\n", argv[i + 1], MAX_TRANSPOSITION_TABLE_SIZE);
                }
            }
            else
            {
                fprintf(stderr, "Warning: --tt-size requires a value\n");
            }
            break;
        }
    }

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

    /* Apply user override if --tt-size was specified */
    if (tt_size_override > 0)
    {
        transposition_table_size = (size_t)tt_size_override;
    }

    transposition_table_init(transposition_table_size);

    int ret_code = 0;

    /* Check if --selfplay is present anywhere in argv (order-independent) */
    int selfplay_mode = 0;
    int selfplay_idx = -1;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--selfplay") == 0 || strcmp(argv[i], "-s") == 0)
        {
            selfplay_mode = 1;
            selfplay_idx = i;
            break;
        }
    }

    if (selfplay_mode)
    {
        int games = 1000;
        int quiet = 0;

        /* Try to parse game count from the argument after --selfplay */
        if (selfplay_idx + 1 < argc)
        {
            char *endp;
            long val = strtol(argv[selfplay_idx + 1], &endp, 10);
            if (endp != argv[selfplay_idx + 1] && *endp == '\0')
            {
                if (val < 1 || val > INT_MAX)
                {
                    fprintf(stderr, "Game count must be a positive integer.\n");
                    transposition_table_free();
                    return 1;
                }
                games = (int)val;
            }
            else if (!((strcmp(argv[selfplay_idx + 1], "--quiet") == 0 || strcmp(argv[selfplay_idx + 1], "-q") == 0) ||
                       (strcmp(argv[selfplay_idx + 1], "--tt-size") == 0 || strcmp(argv[selfplay_idx + 1], "-t") == 0) ||
                       strcmp(argv[selfplay_idx + 1], "--seed") == 0))
            {
                /* Not a valid game count and not a recognized flag, warn */
                fprintf(stderr, "Warning: Invalid --selfplay value '%s', using default %d\n",
                        argv[selfplay_idx + 1], games);
            }
        }

        /* Scan all arguments for flags */
        for (int i = 1; i < argc; ++i)
        {
            if (strcmp(argv[i], "--quiet") == 0 || strcmp(argv[i], "-q") == 0)
            {
                quiet = 1;
            }
            else if (strcmp(argv[i], "--tt-size") == 0 || strcmp(argv[i], "-t") == 0)
            {
                /* Already parsed earlier, skip it and its value if present */
                if (i + 1 < argc)
                {
                    /* Only skip next arg if it's not a flag (or is a negative number) */
                    if (argv[i + 1][0] != '-' || (argv[i + 1][0] == '-' && isdigit((unsigned char)argv[i + 1][1])))
                    {
                        i++; /* Skip the value argument */
                    }
                }
            }
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
