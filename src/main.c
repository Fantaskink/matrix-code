#define _XOPEN_SOURCE_EXTENDED 1
#include <locale.h>
#include <wchar.h>
#include <ncursesw/ncurses.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>

#define COLOR_BRIGHT_GREEN 8
#define COLOR_DIMMER_GREEN 9
#define COLOR_DARK_GREEN 10

void handle_winch(int sig);
int init_colors();
wchar_t get_random_symbol();

const wchar_t *matrix_symbols = L"日ﾊﾐﾋｰｳｼﾅﾓﾆｻﾜﾂｵﾘｱﾎﾃﾏｹﾒｴｶｷﾑﾕﾗｾﾈｽﾀﾇﾍ012345789ZT:・.=*+-<>¦｜╌";

int main()
{
    srand(time(NULL));

    setlocale(LC_ALL, "");
    int height, width;

    signal(SIGWINCH, handle_winch);

    initscr();                       // Initialize the ncurses screen
    getmaxyx(stdscr, height, width); // Get updated size
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    if (init_colors() == 1)
    {
        return 1;
    }

    attron(COLOR_PAIR(1));

    char char_matrix[width][height];

    while (1)
    {

        clear(); // Clear screen

        wchar_t symbol[2];
        symbol[0] = get_random_symbol();
        symbol[1] = L'\0';
        mvaddwstr(3, 5, symbol);

            /*
            // mvprintw(0, 0, "Terminal size: %d rows x %d cols", height, width);
            mvprintw(0, 0, "a");
            mvprintw(0, 10, "a");
            attron(COLOR_PAIR(2));
            mvprintw(1, 0, "a");
            mvprintw(2, 0, "a");
            mvprintw(3, 0, "a");
            mvprintw(4, 0, "a");
            attron(COLOR_PAIR(1));
            mvprintw(5, 0, "a");
            */
            // mvprintw(1, 0, "Resize the terminal or press 'q' to quit.");
            refresh();

        int ch = getch();
        if (ch == 'q')
            break;
    }

    endwin(); // Restore normal terminal behavior
    return 0;
}

void handle_winch(int sig)
{
    // Reinitialize ncurses to handle new size
    endwin();
    refresh();
    clear();
}

int init_colors()
{
    if (has_colors() == FALSE)
    {
        endwin();
        printf("Your terminal does not support color\n");
        return 1;
    }

    if (can_change_color() == FALSE)
    {
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

wchar_t get_random_symbol()
{
    const size_t matrix_symbols_len = wcslen(matrix_symbols);
    return matrix_symbols[rand() % matrix_symbols_len];
}