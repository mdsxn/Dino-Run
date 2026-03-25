# Makefile for building the dino game with raylib on linux
COMPILER = gcc
SOURCE = main.c leaderboard.c obstacle_system.c
OUTPUT = dinogame

# linking flags for raylib and other libraries
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lraylib -lGL -lm -lpthread -ldl -lrt -lX11

# rule to build the game
build:
	$(COMPILER) $(SOURCE) -o $(OUTPUT) $(CFLAGS) $(LDFLAGS)

# clean up rules
clean:
	rm -f $(OUTPUT)