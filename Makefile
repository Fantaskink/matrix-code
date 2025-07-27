# Makefile for compiling with ncursesw from Homebrew

CC = gcc
SRC = src/main.c
OUT = matrix

# Homebrew ncursesw paths (adjust if needed)
NCURSES_PREFIX = /opt/homebrew/opt/ncurses
CFLAGS = -I$(NCURSES_PREFIX)/include
LDFLAGS = -L$(NCURSES_PREFIX)/lib
LIBS = -lncursesw

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(SRC) -o $(OUT) $(CFLAGS) $(LDFLAGS) $(LIBS)

run: all
	./$(OUT)

clean:
	rm -f $(OUT)