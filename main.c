///////////////////////////////////////
/// 640x480 version!
/// This code will segfault the original
/// DE1 computer
/// compile with
/// gcc tetris.c -o tetris -O2 -lm
///
///////////////////////////////////////
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
#include "address_map_arm_brl4.h"
#include "tetris_logic.h"
#include "vga_display.h"

// pixel macro
#define VGA_PIXEL(x,y,color) do{\
	char  *pixel_ptr ;\
	pixel_ptr = (char *)vga_pixel_ptr + ((y)<<10) + (x) ;\
	*(char *)pixel_ptr = (color);\
} while(0)

//light weight bus base
void *h2p_lw_virtual_base;

// pixel buffer
volatile unsigned int * vga_pixel_ptr = NULL ;
void *vga_pixel_virtual_base;

// character buffer
volatile unsigned int * vga_char_ptr = NULL ;
void *vga_char_virtual_base;

// /dev/mem file id
int fd;

// shared memory 
key_t mem_key=0xf0;
int shared_mem_id; 
int *shared_ptr;
int shared_time;
int shared_note;
char shared_str[64];

/*// measure time
struct timeval t1, t2;
double elapsedTime;*/
	
int main(void)
{
	// Declare volatile pointers to I/O registers (volatile 	// means that IO load and store instructions will be used 	// to access these pointer locations, 
	// instead of regular memory loads and stores) 

	// === shared memory =======================
	// with video process
	shared_mem_id = shmget(mem_key, 100, IPC_CREAT | 0666);
 	//shared_mem_id = shmget(mem_key, 100, 0666);
	shared_ptr = shmat(shared_mem_id, NULL, 0);

	// === need to mmap: =======================
	// FPGA_CHAR_BASE
	// FPGA_ONCHIP_BASE      
	// HW_REGS_BASE        
  
	// === get FPGA addresses ==================
    // Open /dev/mem to write to FPGA Memory
	if( ( fd = open( "/dev/mem", ( O_RDWR | O_SYNC ) ) ) == -1 ) 	{
		printf( "ERROR: could not open \"/dev/mem\"...\n" );
		return( 1 );
	}
    
    // get virtual addr that maps to physical
	h2p_lw_virtual_base = mmap( NULL, HW_REGS_SPAN, ( PROT_READ | PROT_WRITE ), MAP_SHARED, fd, HW_REGS_BASE );	
	if( h2p_lw_virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap1() failed...\n" );
		close( fd );
		return(1);
	}

	// === get VGA char addr =====================
	// get virtual addr that maps to physical
	vga_char_virtual_base = mmap( NULL, FPGA_CHAR_SPAN, ( 	PROT_READ | PROT_WRITE ), MAP_SHARED, fd, FPGA_CHAR_BASE );	
	if( vga_char_virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap2() failed...\n" );
		close( fd );
		return(1);
	}
    
    // Get the address that maps to the FPGA LED control 
	vga_char_ptr =(unsigned int *)(vga_char_virtual_base);

	// === get VGA pixel addr ====================
	// get virtual addr that maps to physical
	vga_pixel_virtual_base = mmap( NULL, FPGA_ONCHIP_SPAN, ( 	PROT_READ | PROT_WRITE ), MAP_SHARED, fd, 			FPGA_ONCHIP_BASE);	
	if( vga_pixel_virtual_base == MAP_FAILED ) {
		printf( "ERROR: mmap3() failed...\n" );
		close( fd );
		return(1);
	}
    
    // Get the address that maps to the FPGA pixel buffer
	vga_pixel_ptr =(unsigned int *)(vga_pixel_virtual_base);

	// Variables to keep track of score, and initialize score as 0
	char scoreStr[10];
	int score[1];
	score[0] = 0;
	sprintf(scoreStr, "%d", score[0]); 

	// Initialize 20x10 gameboard as a 1D array
	int gameBoard[200] = {0};

	// Initialize currentPiece object to track current game piece
	struct TetrisPiece *currentPiece;

	// Flag to keep track of whether a new piece is needed
	int newPieceNeeded = 0;

	// Variables to track current game speed and points before gameSpeed is updated
	double gameSpeed = 1.0;
	int nextLevel = 700;
	
	// clear the screen write current score
	VGA_box (0, 0, 639, 479, black);
	VGA_text_clear();
	VGA_text (10, 1, "Tetris Game Score:");
	VGA_text (10, 3, scoreStr);

	// Get current terminal attributes
	struct termios orig_term;
    tcgetattr(STDIN_FILENO, &orig_term);

	// Set the seed for rand() based on current time
	srand(time(0));

	// Start Game
	generatePiece(gameBoard, currentPiece);
	drawGameBoard(gameBoard);
	
	// Main Game Loop, breaks when game ends
	while(1) {
		// Handle user input and game piece movement
		readUserKeys(&orig_term, gameBoard, currentPiece, gameSpeed);
		newPieceNeeded = updateGameBoard(gameBoard, currentPiece);

		// Check if a new piece is needed and update score
		if(newPieceNeeded == 1) {
			if(gameOver(currentPiece))
				break;
			updateScore(gameBoard, scoreStr, score);

			// Adjust game speed as score increases
			if(*score >= nextLevel) {
				gameSpeed = gameSpeed - 0.15*gameSpeed;
				nextLevel += 700;
			}		
			// Generate new piece		 
			generatePiece(gameBoard, currentPiece);		
		}
		// Draw updated game board
		drawGameBoard(gameBoard);
		
	}
	
	// Display game over screen upon exiting loop
	gameOverScreen(scoreStr);
			
	return(0);
}


