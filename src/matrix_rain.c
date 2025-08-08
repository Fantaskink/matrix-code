#define _XOPEN_SOURCE_EXTENDED 1
#include <locale.h>
#include <wchar.h>
#include <ncursesw/ncurses.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <wchar.h> // for wcwidth

#define COLOR_WHITE 8
#define COLOR_BRIGHT_GREEN 9
#define COLOR_DIMMER_GREEN 10
#define COLOR_DARK_GREEN 11

#define PAIR_WHITE 1
#define PAIR_BRIGHT_GREEN 2
#define PAIR_DIMMER_GREEN 3
#define PAIR_DARK_GREEN 4

#define MAX_TRAIL_LENGTH 40

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
void draw_symbol(int row, int col, wchar_t ch, int color_pair,
                 wchar_t **glyph_matrix, int max_width, int max_height);
void erase_symbol(int row, int col, wchar_t **glyph_matrix, int max_width);

const wchar_t *matrix_symbols = L"日ﾊﾐﾋｰｳｼﾅﾓﾆｻﾜﾂｵﾘｱﾎﾃﾏｹﾒｴｶｷﾑﾕﾗｾﾈｽﾀﾇﾍ012345789Z:・.=*+-<>¦｜╌";

int main()
{
    srand(time(NULL));

    setlocale(LC_ALL, "");
    int height, width;

    signal(SIGWINCH, handle_winch);

    initscr();
    curs_set(0);
    getmaxyx(stdscr, height, width);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);

    if (init_colors() == 1)
    {
        return 1;
    }

    // dynamically allocate glyph_matrix
    wchar_t **glyph_matrix = malloc(height * sizeof(wchar_t *));
    for (int i = 0; i < height; i++)
    {
        glyph_matrix[i] = malloc(width * sizeof(wchar_t));
        for (int j = 0; j < width; j++)
            glyph_matrix[i][j] = L' ';
    }

    int max_trails = width + width * (height / MAX_TRAIL_LENGTH);
    Trail trails[max_trails];
    int num_trails = 0;

    for (int i = 0; i < max_trails; i++)
    {
        trails[i].active = 0;
    }

    while (1)
    {
        for (size_t i = 0; i < max_trails; i++)
        {
            Trail *current = &trails[i];
            if (!current->active)
                continue;

            int head_row = current->head_row;
            int column = current->column;

            if (head_row < height)
            {
                wchar_t ch = get_random_symbol();
                draw_symbol(head_row, column, ch, PAIR_WHITE, glyph_matrix, width, height);
            }

            if (head_row > 0 && head_row <= height)
            {
                wchar_t ch = glyph_matrix[head_row - 1][column];
                draw_symbol(head_row - 1, column, ch, PAIR_BRIGHT_GREEN, glyph_matrix, width, height);
            }

            if (head_row > 20 && head_row - 20 < height)
            {
                wchar_t ch = glyph_matrix[head_row - 21][column];
                draw_symbol(head_row - 21, column, ch, PAIR_DIMMER_GREEN, glyph_matrix, width, height);
            }

            if (head_row > 30 && head_row - 30 < height)
            {
                wchar_t ch = glyph_matrix[head_row - 31][column];
                draw_symbol(head_row - 31, column, ch, PAIR_DARK_GREEN, glyph_matrix, width, height);
            }

            int tail_row = current->head_row - current->length;
            if (tail_row >= 0 && tail_row < height)
            {
                erase_symbol(tail_row, column, glyph_matrix, width);
            }

            if (tail_row >= height)
            {
                current->active = 0;
                num_trails--;
            }

            current->head_row++;
        }

        // Add new trail if space available
        if (num_trails < max_trails)
        {
            for (int i = 0; i < max_trails; i++)
            {
                if (!trails[i].active)
                {
                    int random_column = rand() % width;

                    // avoid starting in or next to an occupied cell
                    if (glyph_matrix[0][random_column] != L' ' ||
                        (random_column > 0 && glyph_matrix[0][random_column - 1] != L' ') ||
                        (random_column < width - 1 && glyph_matrix[0][random_column + 1] != L' '))
                    {
                        continue;
                    }

                    trails[i].column = random_column;
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
        napms(100);
    }

    endwin();

    for (int i = 0; i < height; i++)
        free(glyph_matrix[i]);
    free(glyph_matrix);

    return 0;
}

void handle_winch(int sig)
{
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
    init_color(COLOR_WHITE, 1000, 1000, 1000);
    init_color(COLOR_BRIGHT_GREEN, 0, 1000, 255);
    init_color(COLOR_DIMMER_GREEN, 0, 560, 67);
    init_color(COLOR_DARK_GREEN, 0, 231, 0);

    init_pair(1, COLOR_WHITE, COLOR_BLACK);
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

void draw_symbol(int row, int col, wchar_t ch, int color_pair,
                 wchar_t **glyph_matrix, int max_width, int max_height)
{
    if (row < 0 || row >= max_height || col < 0 || col >= max_width)
        return;

    int w = wcwidth(ch);
    if (w == 2 && col == max_width - 1) {
        // Can't place wide char at last column
        return;
    }

    attron(COLOR_PAIR(color_pair));
    wchar_t buf[2] = { ch, L'\0' };
    mvaddwstr(row, col, buf);

    glyph_matrix[row][col] = ch;
    if (w == 2) {
        glyph_matrix[row][col + 1] = L'\0'; // mark right half
    }
}

void erase_symbol(int row, int col, wchar_t **glyph_matrix, int max_width)
{
    if (row < 0 || col < 0 || col >= max_width)
        return;

    int w = wcwidth(glyph_matrix[row][col]);
    if (w <= 0) w = 1; // treat nonprintable as single width

    if (w == 2 && col < max_width - 1) {
        mvaddwstr(row, col, L"  "); // erase both halves in one call
        glyph_matrix[row][col] = L' ';
        glyph_matrix[row][col + 1] = L' ';
    } else {
        mvaddwstr(row, col, L" ");
        glyph_matrix[row][col] = L' ';
    }
}