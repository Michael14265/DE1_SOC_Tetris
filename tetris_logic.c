#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ipc.h> 
#include <sys/shm.h> 
#include <sys/mman.h>
#include <sys/time.h> 
#include <math.h>
#include "tetris_logic.h"
#include "vga_display.h"

// Helper function to enable reading of arrow keys in real time
void enableRawMode(struct termios *orig_term) {
    struct termios raw = *orig_term;
    raw.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// Helper function to disable reading of arrow keys in real time
void disableRawMode(struct termios *orig_term) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, orig_term); // Reset terminal
}

// Reads user input for a specified time window before progressing the game
void readUserKeys(struct termios *orig_term, int *gameBoard, TetrisPiece *currentPiece, double timeWindow) {

	enableRawMode(orig_term);           // Enable raw mode

    fd_set readfds;
    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);

    while (1) {
        // Calculate elapsed time
        gettimeofday(&current_time, NULL);
        double elapsed = (current_time.tv_sec - start_time.tv_sec) +
                         (current_time.tv_usec - start_time.tv_usec) / 1000000.0;
        
		// Updates game board based on current time window
        if (elapsed >= timeWindow) {
			break;
        }
        
        // Reset file descriptor set and timeout each loop iteration
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100ms timeout to poll for input

        // Check for input within the 100ms window
        if (select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout) > 0) {
            int key = getchar();
            if (key == 27) { // Escape character for arrow keys
                if (getchar() == '[') { // Confirm it's an arrow key
                    movePiece(gameBoard, currentPiece, getchar()); 
				}
            }
        }
    }

    disableRawMode(orig_term); // Reset terminal to original settings
}

void gameOverScreen(char *scoreStr) {
	
	char finalScoreStr[30];
	sprintf(finalScoreStr,"Final Score: %s", scoreStr);
	VGA_box (0, 0, 639, 479, black);
	VGA_text_clear();
	VGA_text(36, 30, "Game Over!");
	VGA_text(36, 32, finalScoreStr);
}

int gameOver(TetrisPiece *currentPiece) {
	int i;
	for(i = 0; i < 4;i++) {
		if(currentPiece->blocks[i] < 10)
			return 1;
	}
	return 0;
}

void updateScore(int *gameBoard, char *scoreStr, int *score) {
	int i,j,rowFull;
	for(i = 0;i < 20;i++) {
		rowFull = 1;
		for(j = 0;j < 10;j++) {
			if(gameBoard[10*i + j] == 0)
				rowFull = 0;
		}
		if(rowFull) {
			removeRow(gameBoard, i);
			*score += 100;
			sprintf(scoreStr, "%d", score[0]); 
			VGA_text (10, 3, scoreStr);

		}
	}
}

void removeRow(int *gameBoard, int rowNum) {
	int i,j;
	for(i = rowNum; i > 0; i--) {
		for(j = 0; j < 10; j++) {
			gameBoard[10*i + j] = gameBoard[10*(i-1) + j];
		}
	}
	for(i = 0; i < 10;i++)
		gameBoard[i] = 0;
}	

// Turns current piece clockwise by 90 degrees
void turnPiece(TetrisPiece *currentPiece) {
	
	int width, i, j;

	// Sets width variable based on type of piece
	if(currentPiece->type == 1)
		width = 4;
	else if(currentPiece->type == 5)
		width = 2;
	else
		width = 3;

	// Initializes 1D array representing 2D square containing the current piece
	int shapeSquare[width*width];

	// Initializes all indices that contain a block of the current piece to 1, and the rest to 0
	for(i = 0; i < width;i++) {
		for(j = 0; j < width; j++) {
			if(((currentPiece->position + 10*i + j) == currentPiece->blocks[0]) || ((currentPiece->position + 10*i + j) == currentPiece->blocks[1]) || ((currentPiece->position + 10*i + j) == currentPiece->blocks[2]) || ((currentPiece->position + 10*i + j) == currentPiece->blocks[3])) 
 				shapeSquare[width*i + j] = 1;
			else
				shapeSquare[width*i + j] = 0;
		}
	}

	// Rotate array clockwise by 90 degrees, and place the result in newShapeSquare
	int newShapeSquare[width*width];
	for(i = 0;i < width;i++) {
		for(j = 0;j < width;j++) {
			newShapeSquare[width*i + j] = shapeSquare[width*(width-1) - width*j + i];
		}
	}
	// Replaces the currentPiece block indices with those of the rotated piece
	int currentPieceIndex = 0;
	for(i = 0;i < width;i++) {
		for(j = 0;j < width;j++) {
			if(newShapeSquare[width*i + j] == 1) {
				currentPiece->blocks[currentPieceIndex] = 10*i + j + currentPiece->position;
				currentPieceIndex++;
			}	
		}
	}
}

// Small helper function that checks if the next position of the current block 
// is occupied by another one of its own pieces
// Used to simplify logic in moveNotValid
int notCurrentPiece(TetrisPiece *currentPiece, int currentPieceIndex, int moveSpaces) {
	
	int i;	
	for(i = 0; i < 4; i++) {
		if(currentPieceIndex + moveSpaces == currentPiece->blocks[i])
			return 0;
	}
	return 1;
}

// Checks if a move is valid
int moveNotValid(int *gameBoard, TetrisPiece *currentPiece, int moveSpaces) {
	
	int i;

	// Checks that the move doesn't go off the game board, or overlap with an existing piece
	for(i = 0; i < 4;i++) {
		if((currentPiece->blocks[i] + moveSpaces) > 200 ||  ((currentPiece->blocks[i] % 10 == 0) && ((currentPiece->blocks[i] + moveSpaces) % 10 == 9)) || ((currentPiece->blocks[i] % 10 == 9) && ((currentPiece->blocks[i] + moveSpaces) % 10 == 0)) ||(gameBoard[currentPiece->blocks[i] + moveSpaces] != 0 && (notCurrentPiece(currentPiece, currentPiece->blocks[i], moveSpaces) == 1))) {
			return 1;
		}
	}
	return 0;
}

void movePiece(int *gameBoard, TetrisPiece *currentPiece, char key) {
	
	int i;

    if(key == 'A') {
		for(i = 0; i < 4;i++) {	
			gameBoard[currentPiece->blocks[i]] = 0;
		}
		turnPiece(currentPiece);

	}	
    else if(key == 'B') {
		if(moveNotValid(gameBoard,currentPiece,10)) {
			return;
		}
		for(i = 0; i < 4;i++) {
			
			gameBoard[currentPiece->blocks[i]] = 0;
			currentPiece->blocks[i] = currentPiece->blocks[i] + 10;

		}
		currentPiece->position += 10;
	}
    else if(key == 'C') {
		if(moveNotValid(gameBoard,currentPiece,1)) {
			return;
		}

		for(i = 0; i < 4;i++) {	
			gameBoard[currentPiece->blocks[i]] = 0;
			currentPiece->blocks[i] = currentPiece->blocks[i] + 1;

		}
		currentPiece->position += 1;
	}
   	else if(key == 'D') {
		if(moveNotValid(gameBoard,currentPiece,-1)) {
			return;
		}

		for(i = 0; i < 4;i++) {	
			gameBoard[currentPiece->blocks[i]] = 0;
			currentPiece->blocks[i] = currentPiece->blocks[i] - 1;
		}
		currentPiece->position -= 1;
	}
   

	for(i = 0; i < 4;i++) {
					
		gameBoard[currentPiece->blocks[i]] = currentPiece->type;
	
	}

	drawGameBoard(gameBoard);

}
int updateGameBoard(int *gameBoard, TetrisPiece *currentPiece) {
	
	int i;

    if(moveNotValid(gameBoard, currentPiece, 10)) {
			return 1;
	}

	for(i = 0; i < 4;i++) {
			
		gameBoard[currentPiece->blocks[i]] = 0;
		currentPiece->blocks[i] = currentPiece->blocks[i] + 10;
	}
	
	currentPiece->position += 10;

	for(i = 0; i < 4;i++) {
					
		gameBoard[currentPiece->blocks[i]] = currentPiece->type;
	
	}
	return 0;
	
}

void generatePiece(int *gameBoard, TetrisPiece *currentPiece) {
	
	int i, randomNum;
	
	randomNum = rand() % 7;

	if(randomNum == 0) {
		currentPiece->blocks[0] = 0;
		currentPiece->blocks[1] = 1;
		currentPiece->blocks[2] = 2;
		currentPiece->blocks[3] = 3;
		currentPiece->type = 1;
	} else if(randomNum == 1) {
		currentPiece->blocks[0] = 1;
		currentPiece->blocks[1] = 10;
		currentPiece->blocks[2] = 11;
		currentPiece->blocks[3] = 12;
		currentPiece->type = 2;
	} else if(randomNum == 2) {
		currentPiece->blocks[0] = 0;
		currentPiece->blocks[1] = 10;
		currentPiece->blocks[2] = 11;
		currentPiece->blocks[3] = 12;
		currentPiece->type = 3;
	} else if(randomNum == 3) {
		currentPiece->blocks[0] = 1;
		currentPiece->blocks[1] = 2;
		currentPiece->blocks[2] = 10;
		currentPiece->blocks[3] = 11;
		currentPiece->type = 4;
	} else if(randomNum == 4) {
		currentPiece->blocks[0] = 0;
		currentPiece->blocks[1] = 1;
		currentPiece->blocks[2] = 10;
		currentPiece->blocks[3] = 11;
		currentPiece->type = 5;
	} else if(randomNum == 5) {
		currentPiece->blocks[0] = 2;
		currentPiece->blocks[1] = 10;
		currentPiece->blocks[2] = 11;
		currentPiece->blocks[3] = 12;
		currentPiece->type = 6;
	} else if(randomNum == 6) {
		currentPiece->blocks[0] = 0;
		currentPiece->blocks[1] = 1;
		currentPiece->blocks[2] = 11;
		currentPiece->blocks[3] = 12;
		currentPiece->type = 7;
	} 
	
	currentPiece->position = 0;

	for(i = 0; i < 4; i++) {
		gameBoard[currentPiece->blocks[i]] = currentPiece->type;
	}

}

void drawGameBoard(int *gameBoard) {
	int i,j;
	for(j = 0;j < 20;j++) {
		for(i = 0; i < 10;i++) {
			if(gameBoard[10*j + i] == 0) {
			    VGA_rect(220 + 20*i,60 + 20*j,240+20*i,80 + 20*j);
			    VGA_box(221 + 20*i,61 + 20*j,239+20*i,79 + 20*j,black);
			}			

			if(gameBoard[10*j + i] == 1) {
			    VGA_rect(220 + 20*i,60 + 20*j,240+20*i,80 + 20*j);
			    VGA_box(221 + 20*i,61 + 20*j,239+20*i,79 + 20*j,cyan);
			}
	
			if(gameBoard[10*j + i] == 2) {
				VGA_rect(220 + 20*i,60 + 20*j,240+20*i,80 + 20*j);
				VGA_box(221 + 20*i,61 + 20*j,239+20*i,79 + 20*j,magenta);

			}

			if(gameBoard[10*j + i] == 3) {
				VGA_rect(220 + 20*i,60 + 20*j,240+20*i,80 + 20*j);
				VGA_box(221 + 20*i,61 + 20*j,239+20*i,79 + 20*j,blue);

			}

			if(gameBoard[10*j + i] == 4) {
				VGA_rect(220 + 20*i,60 + 20*j,240+20*i,80 + 20*j);
				VGA_box(221 + 20*i,61 + 20*j,239+20*i,79 + 20*j,green);

			}

			if(gameBoard[10*j + i] == 5) {
				VGA_rect(220 + 20*i,60 + 20*j,240+20*i,80 + 20*j);
				VGA_box(221 + 20*i,61 + 20*j,239+20*i,79 + 20*j,yellow);

			}

			if(gameBoard[10*j + i] == 6) {
				VGA_rect(220 + 20*i,60 + 20*j,240+20*i,80 + 20*j);
				VGA_box(221 + 20*i,61 + 20*j,239+20*i,79 + 20*j,orange);

			}

			if(gameBoard[10*j + i] == 7) {
				VGA_rect(220 + 20*i,60 + 20*j,240+20*i,80 + 20*j);
				VGA_box(221 + 20*i,61 + 20*j,239+20*i,79 + 20*j,red);

			}
			
		}
	}
}
