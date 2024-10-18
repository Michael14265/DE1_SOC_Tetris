# Compiler
CC = gcc

# Compiler flags
CFLAGS = -O2 -Wall

# Linker flags
LDFLAGS = -lm

# Source files
SRC = main.c tetris_logic.c vga_display.c

# Object files
OBJ = $(SRC:.c=.o)

# Executable name
EXEC = tetris

# Default target
all: $(EXEC)

# Link object files to create executable
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Compile each .c file into .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up object files and executable
clean:
	rm -f $(OBJ) $(EXEC)

# Rebuild the project from scratch
rebuild: clean all
