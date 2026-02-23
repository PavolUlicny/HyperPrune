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

/* Platform-specific high-resolution timer */
#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#define _POSIX_C_SOURCE 199309L
#include <time.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <errno.h>
#include "TicTacToe/tic_tac_toe.h"
#include "Negamax/negamax.h"
#include "Negamax/transposition.h"

/* Portable high-resolution timer */
#ifdef _MSC_VER
typedef LARGE_INTEGER HiResTimer;

static int timer_get(HiResTimer *t)
{
    return QueryPerformanceCounter(t) ? 0 : -1;
}

static double timer_diff_seconds(const HiResTimer *start, const HiResTimer *end)
{
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq))
        return -1.0;
    return (double)(end->QuadPart - start->QuadPart) / (double)freq.QuadPart;
}
#else
typedef struct timespec HiResTimer;

static int timer_get(HiResTimer *t)
{
    return clock_gettime(CLOCK_MONOTONIC, t);
}

static double timer_diff_seconds(const HiResTimer *start, const HiResTimer *end)
{
    double sec = (double)(end->tv_sec - start->tv_sec);
    double nsec = (double)(end->tv_nsec - start->tv_nsec);
    return sec + (nsec / 1e9);
}
#endif

/*
 * Maximum transposition table size (entry count).
 * This caps the allocation when BOARD_SIZE is large.
 * At 16 bytes per entry: 250M entries = 4 GB (SI). The allocator rounds up
 * to the next power of 2 (268,435,456 entries = 4 GiB actual).
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
    HiResTimer startTime = {0};
    int timing_available = 0;

    if (!quiet)
    {
        if (timer_get(&startTime) != 0)
        {
            fprintf(stderr, "Warning: timer initialization failed, timing stats will be unavailable\n");
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
            HiResTimer endTime;
            if (timer_get(&endTime) != 0)
            {
                fprintf(stderr, "Warning: timer read failed, timing stats unavailable\n");
                timing_available = 0;
            }
            else
            {
                /* Calculate elapsed time in seconds */
                elapsed = timer_diff_seconds(&startTime, &endTime);

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
    /* Scan for --help first so it always wins over other flags */
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            printf("Usage: ttt [OPTIONS]\n\n");
            printf("Play Tic-Tac-Toe against a perfect negamax AI or run self-play simulations.\n");
            printf("Compiled for %dx%d board.\n\n", BOARD_SIZE, BOARD_SIZE);
            printf("OPTIONS:\n");
            printf("  Interactive Mode (default):\n");
            printf("    Start an interactive game against the AI.\n\n");
            printf("  Self-Play Mode:\n");
            printf("    --selfplay, -s [GAMES]    Run self-play simulations (default: 1000 games)\n");
            printf("    --quiet, -q               Suppress output\n\n");
            printf("  Configuration:\n");
            printf("    --tt-size SIZE, -t SIZE   Transposition table size in entries\n");
            printf("                              (0 disables TT, default: auto-sized, max: %d)\n", MAX_TRANSPOSITION_TABLE_SIZE);
            printf("    --seed SEED               PRNG seed for Zobrist keys (default: deterministic)\n\n");
            printf("  Help:\n");
            printf("    --help, -h                Show this help message and exit\n\n");
            printf("EXAMPLES:\n");
            printf("  ttt                          # Interactive game\n");
            printf("  ttt --selfplay 5000          # Run 5000 self-play games\n");
            printf("  ttt --selfplay 10000 -q      # Run 10000 games, quiet output\n");
            printf("  ttt --seed 42 -s 1000        # Deterministic game with seed 42\n");
            printf("  ttt --tt-size 0 -s 1000      # Benchmark without transposition table\n");
            return 0;
        }
    }

    /* Single-pass argument parsing */
    int opt_selfplay = 0;
    int opt_selfplay_games = 1000;
    int opt_quiet = 0;
    int opt_seed_set = 0;
    uint64_t opt_seed = 0;
    int opt_tt_size_set = 0;
    size_t opt_tt_size = 0;

    for (int i = 1; i < argc; i++)
    {
        const char *arg = argv[i];

        if (strcmp(arg, "--") == 0)
            break; /* Stop option processing; remaining args are positional */

        if (strcmp(arg, "--selfplay") == 0 || strcmp(arg, "-s") == 0)
        {
            opt_selfplay = 1;
            /* Optional positional argument: game count */
            if (i + 1 < argc)
            {
                char *endptr;
                errno = 0;
                long val = strtol(argv[i + 1], &endptr, 10);
                if (endptr != argv[i + 1] && *endptr == '\0')
                {
                    /* Valid integer — consume as game count */
                    if (errno == ERANGE || val < 1 || val > INT_MAX)
                    {
                        fprintf(stderr, "Game count must be a positive integer.\n");
                        return EXIT_FAILURE;
                    }
                    opt_selfplay_games = (int)val;
                    i++;
                }
                else if (argv[i + 1][0] != '-')
                {
                    /* Non-flag, non-integer: warn and leave in place */
                    fprintf(stderr, "Warning: Invalid --selfplay value '%s', using default %d\n",
                            argv[i + 1], opt_selfplay_games);
                }
                /* Starts with '-': another flag, leave for next iteration */
            }
        }
        else if (strcmp(arg, "--quiet") == 0 || strcmp(arg, "-q") == 0)
        {
            opt_quiet = 1;
        }
        else if (strcmp(arg, "--seed") == 0)
        {
            if (i + 1 >= argc)
            {
                fprintf(stderr, "Error: --seed requires a value\n");
                return EXIT_FAILURE;
            }
            i++;
            const char *seed_str = argv[i];
            if (seed_str[0] == '\0')
            {
                fprintf(stderr, "Error: --seed value cannot be empty\n");
                return EXIT_FAILURE;
            }
            if (seed_str[0] == '-')
            {
                fprintf(stderr, "Error: Invalid --seed value '%s' (must be 0 to %llu)\n",
                        seed_str, (unsigned long long)ULLONG_MAX);
                return EXIT_FAILURE;
            }
            char *endptr;
            errno = 0;
            unsigned long long val = strtoull(seed_str, &endptr, 10);
            if (endptr == seed_str || *endptr != '\0')
            {
                fprintf(stderr, "Error: Invalid --seed value '%s' (not a valid number)\n", seed_str);
                return EXIT_FAILURE;
            }
            if (errno == ERANGE)
            {
                fprintf(stderr, "Error: --seed value '%s' out of range (max: %llu)\n",
                        seed_str, (unsigned long long)ULLONG_MAX);
                return EXIT_FAILURE;
            }
            opt_seed = (uint64_t)val;
            opt_seed_set = 1;
        }
        else if (strcmp(arg, "--tt-size") == 0 || strcmp(arg, "-t") == 0)
        {
            if (i + 1 >= argc ||
                (argv[i + 1][0] == '-' && !isdigit((unsigned char)argv[i + 1][1])))
            {
                fprintf(stderr, "Error: --tt-size requires a value\n");
                return EXIT_FAILURE;
            }
            i++;
            char *endptr;
            errno = 0;
            long val = strtol(argv[i], &endptr, 10);
            if (endptr == argv[i] || *endptr != '\0' ||
                errno == ERANGE || val < 0 || val > MAX_TRANSPOSITION_TABLE_SIZE)
            {
                fprintf(stderr, "Error: Invalid --tt-size value '%s' (must be 0 to %d)\n",
                        argv[i], MAX_TRANSPOSITION_TABLE_SIZE);
                return EXIT_FAILURE;
            }
            opt_tt_size = (size_t)val;
            opt_tt_size_set = 1;
        }
        else if (arg[0] == '-')
        {
            fprintf(stderr, "Error: Unknown option '%s'\n", arg);
            fprintf(stderr, "Use --help to see available options.\n");
            return EXIT_FAILURE;
        }
        /* Non-flag positional args not consumed as values are silently ignored */
    }

    /* Initialize subsystems in required order */
    init_win_masks();
    if (opt_seed_set)
        zobrist_set_seed(opt_seed);
    zobrist_init();

    /*
     * Dynamic transposition table sizing.
     * Formula: size = 1,500,000 × (BOARD_SIZE / 4)^9.4
     * The result is capped at MAX_TRANSPOSITION_TABLE_SIZE.
     */
    size_t tt_size;

#if BOARD_SIZE == 3
    tt_size = 100000;
#elif BOARD_SIZE == 4
    tt_size = 1500000;
#else
    {
        double growth_factor = pow((double)BOARD_SIZE / 4.0, 9.4);
        tt_size = (size_t)(1500000.0 * growth_factor);
        if (tt_size > MAX_TRANSPOSITION_TABLE_SIZE)
            tt_size = MAX_TRANSPOSITION_TABLE_SIZE;
    }
#endif

    if (opt_tt_size_set)
        tt_size = opt_tt_size;

    transposition_table_init(tt_size);

    int ret_code = 0;
    if (opt_selfplay)
        ret_code = selfPlay(opt_selfplay_games, opt_quiet);
    else
        playGame();

    transposition_table_free();
    return ret_code;
}
