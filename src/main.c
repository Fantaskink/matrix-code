#define _XOPEN_SOURCE_EXTENDED 1
#include <locale.h>
#include <wchar.h>
#include <ncursesw/ncurses.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>

#define COLOR_VERY_BRIGHT_GREEN 8
#define COLOR_BRIGHT_GREEN 9
#define COLOR_DIMMER_GREEN 10
#define COLOR_DARK_GREEN 11

void handle_winch(int sig);
int init_colors();
wchar_t get_random_symbol();

const wchar_t *matrix_symbols = L"日ﾊﾐﾋｰｳｼﾅﾓﾆｻﾜﾂｵﾘｱﾎﾃﾏｹﾒｴｶｷﾑﾕﾗｾﾈｽﾀﾇﾍ012345789Z:・.=*+-<>¦｜╌";

typedef struct
{
    wchar_t symbol;
    int age;
    int active;
} Glyph;

int main()
{
    srand(time(NULL));

    setlocale(LC_ALL, "");
    int height, width;

    signal(SIGWINCH, handle_winch);

    initscr();                       // Initialize the ncurses screen
    curs_set(0);                     // 0 = invisible, 1 = normal, 2 = very visible (if supported)
    getmaxyx(stdscr, height, width); // Get updated size
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    if (init_colors() == 1)
    {
        return 1;
    }

    Glyph glyph_matrix[height][width];

    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            glyph_matrix[i][j].active = 0;
        }
    }

    while (1)
    {
        clear();

        // if (rand() % 2 == 1) // 25% chance of spawning a new glyph
        if (1)
        {
            int random_x = rand() % width;
            if (glyph_matrix[0][random_x].active == 0)
            {
                glyph_matrix[0][random_x].active = 1;
                glyph_matrix[0][random_x].age = 0;
                glyph_matrix[0][random_x].symbol = get_random_symbol();
            }
        }

        for (int i = height - 1; i >= 0; i--)
        {
            for (size_t j = 0; j < width; j++)
            {
                Glyph *current = &glyph_matrix[i][j];
                if (!current->active)
                {
                    continue; // Skip inactive glyphs
                }

                if (current->age == 0)
                {
                    attron(COLOR_PAIR(1)); // Set white color for glyph
                    
                    if (i < height - 1) // Only activate glyph below if not at bottom row
                    {
                        glyph_matrix[i + 1][j].active = 1; // Otherwise, activate glyph below
                        glyph_matrix[i + 1][j].symbol = get_random_symbol();
                        glyph_matrix[i + 1][j].age = 0;
                    }
                }

                if (current->age > 0)
                {
                    attron(COLOR_PAIR(2)); // Set bright green color for glyph
                }

                if (current->age > 20)
                {
                    attron(COLOR_PAIR(3)); // Set darker green color for glyph
                }

                if (current->age > 30)
                {
                    attron(COLOR_PAIR(4)); // Set darker green color for glyph
                }

                if (current->age > height)
                {
                    current->active = 0;
                }

                wchar_t symbol[2];
                symbol[0] = current->symbol;
                symbol[1] = L'\0';
                mvaddwstr(i, j, symbol);

                current->age++;
            }
        }

        refresh();
        napms(100); // Delay for 0.1 seconds
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
    init_color(COLOR_VERY_BRIGHT_GREEN, 700, 1000, 780);
    init_color(COLOR_BRIGHT_GREEN, 0, 1000, 255);
    init_color(COLOR_DIMMER_GREEN, 0, 560, 67);
    init_color(COLOR_DARK_GREEN, 0, 231, 0);

    init_pair(1, COLOR_VERY_BRIGHT_GREEN, COLOR_BLACK);
    init_pair(2, COLOR_BRIGHT_GREEN, COLOR_BLACK);
    init_pair(3, COLOR_DIMMER_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_DARK_GREEN, COLOR_BLACK);

    return 0;
}

wchar_t get_random_symbol()
{
    const size_t matrix_symbols_len = wcslen(matrix_symbols);
    return matrix_symbols[rand() % matrix_symbols_len];
}