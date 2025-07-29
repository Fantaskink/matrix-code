#define _XOPEN_SOURCE_EXTENDED 1
#define _GNU_SOURCE
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

#define MAX_TRAIL_LENGTH 30

typedef struct
{
    int column;
    int head_row;
    int length;
    int max_length;
    int active;
} Trail;

void handle_winch(int sig);
int init_colors();
wchar_t get_random_symbol();

const wchar_t *matrix_symbols = L"日ﾊﾐﾋｰｳｼﾅﾓﾆｻﾜﾂｵﾘｱﾎﾃﾏｹﾒｴｶｷﾑﾕﾗｾﾈｽﾀﾇﾍ012345789Z:・.=*+-<>¦｜╌";

int main()
{
    srand(time(NULL));

    setlocale(LC_ALL, "");
    int height, width;

    signal(SIGWINCH, handle_winch);

    initscr();
    curs_set(0);                     // 0 = invisible, 1 = normal, 2 = very visible (if supported)
    getmaxyx(stdscr, height, width); // Get terminal window size
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    if (init_colors() == 1)
    {
        return 1;
    }

    wchar_t glyph_matrix[height][width];
    
    // Initialize glyph_matrix with spaces
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            glyph_matrix[i][j] = L' ';
        }
    }

    /* Trails for a full screen width + extra if the screen has space for multiple trails
       in a column */
    int max_trails = width + width * (height / MAX_TRAIL_LENGTH);
    Trail trails[max_trails];
    int num_trails = 0;
    
    // Initialize all trails as inactive
    for (int i = 0; i < max_trails; i++)
    {
        trails[i].active = 0;
    }

    wchar_t symbol[2];
    symbol[1] = L'\0';

    while (1)
    {
        for (size_t i = 0; i < max_trails; i++)
        {
            Trail *current = &trails[i];
            
            // Skip inactive trails
            if (!current->active)
                continue;
                
            int head_row = current->head_row;
            int column = current->column;

            if (head_row < height)
            {
                attron(COLOR_PAIR(1)); // Set white color for trail head glyph
                symbol[0] = get_random_symbol();
                mvaddwstr(head_row, column, symbol);
                glyph_matrix[head_row][column] = symbol[0];
            }

            if (head_row > 0 && head_row <= height)
            {
                // Render glyph above trail head with green color
                attron(COLOR_PAIR(2)); // Set green color for glyph above head
                symbol[0] = glyph_matrix[head_row - 1][column];
                mvaddwstr(head_row - 1, column, symbol);
            }

            int tail_row = current->head_row - current->length;
            if (tail_row >= 0 && tail_row < height)
            {
                mvaddwstr(tail_row, column, L" ");
            }

            if (tail_row >= height)
            {
                current->active = 0;
                num_trails--;
            }

            current->head_row++;
        }

        // Only add new trail if we have space and randomly
        if (num_trails < max_trails && rand() % 10 == 0)
        {
            // Find first inactive slot
            for (int i = 0; i < max_trails; i++)
            {
                if (!trails[i].active)
                {
                    trails[i].column = rand() % width;
                    trails[i].head_row = 0;
                    trails[i].length = MAX_TRAIL_LENGTH;
                    trails[i].max_length = MAX_TRAIL_LENGTH;
                    trails[i].active = 1;
                    num_trails++;
                    break;
                }
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
    init_color(COLOR_VERY_BRIGHT_GREEN, 1000, 1000, 1000);
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