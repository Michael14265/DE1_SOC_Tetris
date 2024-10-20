#ifndef TETRIS_LOGIC_H
#define TETRIS_LOGIC_H

#include <termios.h>

typedef struct {
	int blocks[4];
	int type;
	int position;
} TetrisPiece;

/**
 * Enable raw mode for terminal, disabling line buffering and echo.
 *
 * Parameters:
 *   - orig_term: A pointer to the termios structure storing original terminal settings.
 */
void enableRawMode(struct termios *orig_term);

/**
 * Disable raw mode and restore original terminal settings.
 *
 * Parameters:
 *   - orig_term: A pointer to the termios structure storing original terminal settings.
 */
void disableRawMode(struct termios *orig_term);

/**
 * Reads user input keys and processes movement and rotation for game pieces.
 *
 * Parameters:
 *   - orig_term: A pointer to the termios structure storing original terminal settings.
 *   - gameBoard: A pointer to the game board array (int array of size 200).
 *   - currentPiece: A pointer to the current piece's position and state (int array of size 6).
 *   - timeWindow: The allowed time window for user input capture in seconds.
 */
void readUserKeys(struct termios *orig_term, int *gameBoard, TetrisPiece *piece, double timeWindow);

// Tetris Logic Function Definitions
int gameOver(TetrisPiece *);
int notCurrentPiece(TetrisPiece *, int, int);
int moveNotValid(int *, TetrisPiece *, int);
void turnPiece(TetrisPiece *);
void movePiece(int *, TetrisPiece *, char);
void generatePiece(int *, TetrisPiece *);
void drawGameBoard(int *);
int updateGameBoard(int *, TetrisPiece *);
void updateScore(int *, char *, int *);
void removeRow(int *, int);
void gameOverScreen(char *);

#endif // TETRIS_LOGIC_H
