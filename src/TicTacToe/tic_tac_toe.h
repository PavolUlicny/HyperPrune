/*
 * Tic-Tac-Toe board and I/O utilities
 * -----------------------------------
 * Public API for handling game state, user interaction, and simple result
 * evaluation. The engine logic (minimax) is in MiniMax/.
 *
 * Board representation:
 *  - 2D char array board[BOARD_SIZE][BOARD_SIZE]
 *  - 'x' / 'o' for players, ' ' (space) for empty
 *
 * Configuration:
 *  - BOARD_SIZE (default 3, can be overridden at compile-time)
 *  - MAX_MOVES = BOARD_SIZE * BOARD_SIZE
 */

#ifndef TIC_TAC_TOE_H
#define TIC_TAC_TOE_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Size of the square board; override via -DBOARD_SIZE=N at compile time. */
#ifndef BOARD_SIZE
#define BOARD_SIZE 3
#endif

#define MAX_MOVES ((BOARD_SIZE) * (BOARD_SIZE))

    /** Game outcome from the point-of-view of the UI. */
    typedef enum
    {
        GAME_CONTINUE = 0,
        PLAYER_WIN = 1,
        AI_WIN = 2,
        GAME_TIE = 3
    } GameResult;

    /* Global game state (simple CLI program design). */
    extern char board[BOARD_SIZE][BOARD_SIZE];
    extern char player_turn;  /* whose turn it is: 'x' or 'o' */
    extern char human_symbol; /* player's chosen symbol */
    extern char ai_symbol;    /* AI's symbol (opposite of human_symbol) */

    /** Set all board cells to ' ' (empty). */
    void initializeBoard(void);

    /** Pretty-print the current board to stdout with row/column indices. */
    void printBoard(void);

    /** Print a human-readable message and board for a terminal result. */
    void printGameResult(GameResult result);

    /**
     * Read a legal move from stdin as (col,row) in 1-based input.
     * Blocks until a valid empty cell is provided or EOF is encountered.
     * Returns 0-based coordinates via out_row/out_col.
     */
    void getMove(int *out_row, int *out_col);

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

#ifdef __cplusplus
}
#endif

#endif
