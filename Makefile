# Makefile for compiling with ncursesw from Homebrew

CC = gcc
SRC = src/matrix_rain.c src/ini_parser.c
OUT = matrix

# Homebrew ncursesw paths (adjust if needed)
NCURSES_PREFIX = /opt/homebrew/opt/ncurses
INIH_PREFIX = /opt/homebrew/opt/inih
CFLAGS = -Wall -Werror -O2 -DNCURSES_WIDECHAR=1 -I$(NCURSES_PREFIX)/include -I$(INIH_PREFIX)/include -Iinclude
LDFLAGS = -L$(NCURSES_PREFIX)/lib -L$(INIH_PREFIX)/lib
LIBS = -lncursesw -linih

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(SRC) -o $(OUT) $(CFLAGS) $(LDFLAGS) $(LIBS)

run: all
	./$(OUT)

clean:
	rm -f $(OUT)