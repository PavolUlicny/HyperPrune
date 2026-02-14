/*
 * Tic-Tac-Toe board and I/O utilities
 * -----------------------------------
 * Public API for handling game state, user interaction, and simple result
 * evaluation. The engine logic (minimax) is in MiniMax/.
 *
 * Board representation:
 *  - Bitboard structure with two uint64_t (x_pieces, o_pieces)
 *  - Each bit represents a board position (row * BOARD_SIZE + col)
 *  - 'x' / 'o' for players, empty cells have no bits set
 *
 * Configuration:
 *  - BOARD_SIZE (default 3, can be overridden at compile-time, max 8)
 *  - MAX_MOVES = BOARD_SIZE * BOARD_SIZE
 */

#ifndef TIC_TAC_TOE_H
#define TIC_TAC_TOE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* Size of the square board; override via -DBOARD_SIZE=N at compile time. */
#ifndef BOARD_SIZE
#define BOARD_SIZE 3
#endif

/* Bitboards support 3x3 to 8x8 boards (64 bits in uint64_t) */
#if BOARD_SIZE < 3
#error "BOARD_SIZE < 3 is not supported (minimum is 3x3)"
#endif

#if BOARD_SIZE > 8
#error "BOARD_SIZE > 8 is not supported with bitboard implementation (max 64 positions)"
#endif

#define MAX_MOVES ((BOARD_SIZE) * (BOARD_SIZE))

    /* Bitboard representation: two uint64_t bitboards for x and o pieces */
    typedef struct
    {
        uint64_t x_pieces; /* Bitboard for 'x' player */
        uint64_t o_pieces; /* Bitboard for 'o' player */
    } Bitboard;

/* Bitboard helper macros */
#define POS_TO_BIT(row, col) ((row) * BOARD_SIZE + (col))
#define BIT_TO_ROW(bit) ((bit) / BOARD_SIZE)
#define BIT_TO_COL(bit) ((bit) % BOARD_SIZE)
#define BIT_MASK(row, col) (1ULL << POS_TO_BIT(row, col))

    /** Game outcome from the point-of-view of the UI. */
    typedef enum
    {
        GAME_CONTINUE = 0,
        PLAYER_WIN = 1,
        AI_WIN = 2,
        GAME_TIE = 3
    } GameResult;

    /* Global game state (simple CLI program design). */
    extern Bitboard board_state;
    extern char player_turn;  /* whose turn it is: 'x' or 'o' */
    extern char human_symbol; /* player's chosen symbol */
    extern char ai_symbol;    /* AI's symbol (opposite of human_symbol) */

    /**
     * Initialize win detection masks for bitboard operations.
     * Must be called once at program startup before any bitboard win checks.
     */
    void init_win_masks(void);

    /**
     * Set all board cells to empty.
     * NOTE: Only resets bitboard state. Does NOT reset move_count or player_turn.
     * For complete game reset, use restartGame() instead.
     */
    void initializeBoard(void);

    /** Pretty-print the current board to stdout with row/column indices. */
    void printBoard(void);

    /** Print a human-readable message and board for a terminal result. */
    void printGameResult(GameResult result);

    /**
     * Read a legal move from stdin as (col,row) in 1-based input.
     * Blocks until a valid empty cell is provided or EOF is encountered.
     * Returns 0-based coordinates via out_row/out_col.
     * Returns:
     *   0 on success (move stored in output parameters)
     *  -1 on EOF (user wants to quit)
     */
    int getMove(int *out_row, int *out_col);

    /** Apply a move for current player_turn and flip the turn. */
    void makeMove(int row, int col);

    /**
     * Check if the last move at (row,col) decided the game.
     * Returns PLAYER_WIN or AI_WIN based on the symbol at that cell,
     * GAME_TIE if the board is full, or GAME_CONTINUE otherwise.
     */
    GameResult checkWinner(int row, int col);

    /** Reset board, move counter, and player_turn to initial state. */
    void restartGame(void);

    /** Ask the user whether to restart; returns 1 for yes, 0 for no. */
    int askRestart(void);

    /** Prompt the user to choose 'x' or 'o' and set human_symbol/ai_symbol. */
    void choosePlayerSymbol(void);

    /* Bitboard utility functions */

    /** Make a move on the bitboard (set bit for player). */
    static inline void bitboard_make_move(Bitboard *board, int row, int col, char player)
    {
        uint64_t mask = BIT_MASK(row, col);
        if (player == 'x')
            board->x_pieces |= mask;
        else
            board->o_pieces |= mask;
    }

    /** Unmake a move on the bitboard (clear bit for player). */
    static inline void bitboard_unmake_move(Bitboard *board, int row, int col, char player)
    {
        uint64_t mask = BIT_MASK(row, col);
        if (player == 'x')
            board->x_pieces &= ~mask;
        else
            board->o_pieces &= ~mask;
    }

    /** Get the symbol at a cell ('x', 'o', or ' '). */
    static inline char bitboard_get_cell(Bitboard board, int row, int col)
    {
        uint64_t mask = BIT_MASK(row, col);
        if (board.x_pieces & mask)
            return 'x';
        if (board.o_pieces & mask)
            return 'o';
        return ' ';
    }

    /** Check if a cell is empty. */
    static inline int bitboard_is_empty(Bitboard board, int row, int col)
    {
        uint64_t mask = BIT_MASK(row, col);
        return !((board.x_pieces | board.o_pieces) & mask);
    }

    /**
     * Check if a player has won (full board scan).
     * Uses pre-computed win masks.
     */
    int bitboard_has_won(uint64_t player_pieces);

    /**
     * Win check based on last move.
     * Only checks relevant patterns (row, col, diagonals if applicable).
     */
    int bitboard_did_last_move_win(uint64_t player_pieces, int row, int col);

#ifdef __cplusplus
}
#endif

#endif
