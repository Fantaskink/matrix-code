#include <ncurses.h>
#include <signal.h>
#include <stdlib.h>

#define COLOR_BRIGHT_GREEN 8
#define COLOR_DIMMER_GREEN 9
#define COLOR_DARK_GREEN 10

void handle_winch(int sig);
int init_colors();

int main() {
    int height, width;

    signal(SIGWINCH, handle_winch);

    initscr();              // Initialize the ncurses screen
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    if (init_colors() == 1) {
        return 1;
    }
    

    attron(COLOR_PAIR(3));

    while (1) {
        getmaxyx(stdscr, height, width);  // Get updated size
        clear();  // Clear screen
        //mvprintw(0, 0, "Terminal size: %d rows x %d cols", height, width);
        mvprintw(0, 0, "a");
        mvprintw(0, 10, "a");
        attron(COLOR_PAIR(2));
        mvprintw(1, 0, "a");
        mvprintw(2, 0, "a");
        mvprintw(3, 0, "a");
        mvprintw(4, 0, "a");
        attron(COLOR_PAIR(1));
        mvprintw(5, 0, "a");
        //mvprintw(1, 0, "Resize the terminal or press 'q' to quit.");
        refresh();

        int ch = getch();
        if (ch == 'q') break;
    }

    endwin();               // Restore normal terminal behavior
    return 0;
}

void handle_winch(int sig) {
    // Reinitialize ncurses to handle new size
    endwin();
    refresh();
    clear();
}

int init_colors() {

    if(has_colors() == FALSE)
	{	endwin();
		printf("Your terminal does not support color\n");
		return 1;
	}

    if(can_change_color() == FALSE) {
        endwin();
        printf("Your terminal does not support changing color\n");
        return 1;
    }

    start_color();
    init_color(COLOR_BLACK, 0, 0, 0);
    init_color(COLOR_BRIGHT_GREEN, 0, 1000, 255);
    init_color(COLOR_DIMMER_GREEN, 0, 560, 67);
    init_color(COLOR_DARK_GREEN, 0, 231, 0);
    

    init_pair(1, COLOR_BRIGHT_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_DIMMER_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_DARK_GREEN, COLOR_BLACK);

    return 0;
}