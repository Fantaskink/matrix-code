#define _XOPEN_SOURCE_EXTENDED 1
#include <locale.h>
#include <wchar.h>
#include <ncursesw/ncurses.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#define COLOR_BRIGHT_GREEN 8
#define COLOR_DIMMER_GREEN 9
#define COLOR_DARK_GREEN 10

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
    int message_char;
} Trail;

void handle_winch(int sig);
int init_colors();
wchar_t get_random_symbol();
void draw_symbol(int row, int col, wchar_t ch, int color_pair,
                 wchar_t **glyph_matrix, int max_width, int max_height);
void erase_symbol(int row, int col, wchar_t **glyph_matrix, int max_width);
int is_message_column(int message_len, int column, int *message_columns);
wchar_t get_message_char(int message_len, int column, int *message_columns, wchar_t *message);

const wchar_t *matrix_symbols = L"日ﾊﾐﾋｰｳｼﾅﾓﾆｻﾜﾂｵﾘｱﾎﾃﾏｹﾒｴｶｷﾑﾕﾗｾﾈｽﾀﾇﾍ012345789Z:・.=*+-<>¦｜╌";

int main()
{
    srand((unsigned)time(NULL));

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

    // TODO allow any message
    // TODO ensure that message does not exceed screen width
    wchar_t message[] = L"SKINK SYSTEMS";
    int message_len = wcslen(message);
    int middle_row = height / 2;
    int message_columns[message_len];
    int message_revealed[message_len]; // Track which message characters have been revealed

    int leftmost_column = (width / 2) - (message_len / 2);

    for (int i = 0; i < message_len; i++)
    {
        message_columns[i] = leftmost_column + i;
        message_revealed[i] = 0; // Initially no characters are revealed
    }

    // dynamically allocate glyph_matrix
    wchar_t **glyph_matrix = malloc(height * sizeof(wchar_t *));
    if (!glyph_matrix)
    {
        endwin();
        return 1;
    }
    for (int i = 0; i < height; i++)
    {
        glyph_matrix[i] = malloc(width * sizeof(wchar_t));
        if (!glyph_matrix[i])
        {
            for (int k = 0; k < i; k++)
                free(glyph_matrix[k]);
            free(glyph_matrix);
            endwin();
            return 1;
        }
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
        for (size_t i = 0; i < (size_t)max_trails; i++)
        {
            Trail *current = &trails[i];
            if (!current->active)
                continue;

            int head_row = current->head_row;
            int column = current->column;

            /* HEAD - reveal message character if head passes over it */
            if (head_row >= 0 && head_row < height)
            {
                if (head_row == middle_row && is_message_column(message_len, column, message_columns))
                {
                    // Reveal the message character at this position
                    for (int j = 0; j < message_len; j++)
                    {
                        if (message_columns[j] == column)
                        {
                            message_revealed[j] = 1;
                            break;
                        }
                    }
                    // Draw the revealed message character
                    wchar_t ch = get_message_char(message_len, column, message_columns, message);
                    draw_symbol(head_row, column, ch, PAIR_WHITE, glyph_matrix, width, height);
                }
                else
                {
                    wchar_t ch = get_random_symbol();
                    draw_symbol(head_row, column, ch, PAIR_WHITE, glyph_matrix, width, height);
                }
            }

            /* BODY: immediate above head - but skip if it would overwrite revealed message */
            int r = head_row - 1;
            if (r >= 0 && r < height)
            {
                if (r == middle_row && is_message_column(message_len, column, message_columns))
                {
                    // Check if this message character is revealed
                    int is_revealed = 0;
                    for (int j = 0; j < message_len; j++)
                    {
                        if (message_columns[j] == column && message_revealed[j])
                        {
                            is_revealed = 1;
                            break;
                        }
                    }
                    if (!is_revealed)
                    {
                        // Not revealed yet, draw normal trail character
                        wchar_t ch = glyph_matrix[r][column];
                        draw_symbol(r, column, ch, PAIR_BRIGHT_GREEN, glyph_matrix, width, height);
                    }
                }
                else
                {
                    wchar_t ch = glyph_matrix[r][column];
                    draw_symbol(r, column, ch, PAIR_BRIGHT_GREEN, glyph_matrix, width, height);
                }
            }

            /* DIMMER: 21 rows above head - but skip if it would overwrite revealed message */
            r = head_row - 21;
            if (r >= 0 && r < height)
            {
                if (r == middle_row && is_message_column(message_len, column, message_columns))
                {
                    // Check if this message character is revealed
                    int is_revealed = 0;
                    for (int j = 0; j < message_len; j++)
                    {
                        if (message_columns[j] == column && message_revealed[j])
                        {
                            is_revealed = 1;
                            break;
                        }
                    }
                    if (!is_revealed)
                    {
                        // Not revealed yet, draw normal trail character
                        wchar_t ch = glyph_matrix[r][column];
                        draw_symbol(r, column, ch, PAIR_DIMMER_GREEN, glyph_matrix, width, height);
                    }
                }
                else
                {
                    wchar_t ch = glyph_matrix[r][column];
                    draw_symbol(r, column, ch, PAIR_DIMMER_GREEN, glyph_matrix, width, height);
                }
            }

            /* DARK: 31 rows above head - but skip if it would overwrite revealed message */
            r = head_row - 31;
            if (r >= 0 && r < height)
            {
                if (r == middle_row && is_message_column(message_len, column, message_columns))
                {
                    // Check if this message character is revealed
                    int is_revealed = 0;
                    for (int j = 0; j < message_len; j++)
                    {
                        if (message_columns[j] == column && message_revealed[j])
                        {
                            is_revealed = 1;
                            break;
                        }
                    }
                    if (!is_revealed)
                    {
                        // Not revealed yet, draw normal trail character
                        wchar_t ch = glyph_matrix[r][column];
                        draw_symbol(r, column, ch, PAIR_DARK_GREEN, glyph_matrix, width, height);
                    }
                }
                else
                {
                    wchar_t ch = glyph_matrix[r][column];
                    draw_symbol(r, column, ch, PAIR_DARK_GREEN, glyph_matrix, width, height);
                }
            }

            int tail_row = current->head_row - current->length;

            if (tail_row >= 0 && tail_row < height)
            {
                // Don't erase revealed message characters
                if (tail_row == middle_row && is_message_column(message_len, column, message_columns))
                {
                    // Check if this message character is revealed
                    int is_revealed = 0;
                    for (int j = 0; j < message_len; j++)
                    {
                        if (message_columns[j] == column && message_revealed[j])
                        {
                            is_revealed = 1;
                            break;
                        }
                    }
                    if (!is_revealed)
                    {
                        // Not revealed yet, can erase
                        erase_symbol(tail_row, column, glyph_matrix, width);
                    }
                }
                else
                {
                    erase_symbol(tail_row, column, glyph_matrix, width);
                }
            }

            if (tail_row >= height)
            {
                current->active = 0;
                num_trails--;
            }

            current->head_row++;
        }

        // Draw only the revealed message characters
        for (int i = 0; i < message_len; i++)
        {
            if (message_revealed[i])
            {
                int msg_col = message_columns[i];
                wchar_t ch = message[i];
                draw_symbol(middle_row, msg_col, ch, PAIR_WHITE, glyph_matrix, width, height);
            }
        }

        // Add new trail if space available
        if (num_trails < max_trails)
        {
            for (int i = 0; i < max_trails; i++)
            {
                if (!trails[i].active)
                {
                    int random_column = rand() % width;

                    // avoid starting in or next to an occupied cell (also treats right-half as occupied)
                    int left_ok = (glyph_matrix[0][random_column] == L' ');
                    int left_left_ok = (random_column > 0) ? (glyph_matrix[0][random_column - 1] == L' ') : 1;
                    int right_ok = (random_column < width - 1) ? (glyph_matrix[0][random_column + 1] == L' ') : 1;

                    if (!left_ok || !left_left_ok || !right_ok)
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
        napms(100); // 0.1 second delay
    }

    endwin();

    for (int i = 0; i < height; i++)
        free(glyph_matrix[i]);
    free(glyph_matrix);

    return 0;
}

void handle_winch(int sig)
{
    // Reinitialize or handle resize in a real program.
    // For simplicity we just call ncurses resize helpers.
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

    init_pair(PAIR_WHITE, COLOR_WHITE, COLOR_BLACK);
    init_pair(PAIR_BRIGHT_GREEN, COLOR_BRIGHT_GREEN, COLOR_BLACK);
    init_pair(PAIR_DIMMER_GREEN, COLOR_DIMMER_GREEN, COLOR_BLACK);
    init_pair(PAIR_DARK_GREEN, COLOR_DARK_GREEN, COLOR_BLACK);

    return 0;
}

wchar_t get_random_symbol()
{
    const size_t matrix_symbols_len = wcslen(matrix_symbols);
    return matrix_symbols[rand() % matrix_symbols_len];
}

/* Draw a symbol at row,col — width-aware and bounds-guarded.
 * For wide chars we also mark the right half in glyph_matrix by storing the same char
 * in both cells. This keeps later reads consistent.
 */
void draw_symbol(int row, int col, wchar_t ch, int color_pair,
                 wchar_t **glyph_matrix, int max_width, int max_height)
{
    if (row < 0 || row >= max_height || col < 0 || col >= max_width)
        return;

    if (ch == L' ' || ch == 0)
        return; // nothing to draw

    int w = wcwidth(ch);
    if (w == 2 && col == max_width - 1)
    {
        // Can't place wide char at last column
        return;
    }

    attron(COLOR_PAIR(color_pair));

    // draw glyph (always write starting at leading cell)
    wchar_t buf[2] = {ch, L'\0'};
    mvaddwstr(row, col, buf);

    // mark matrix: store the same wchar in both halves so later reads are sane
    glyph_matrix[row][col] = ch;
    if (w == 2 && col + 1 < max_width)
    {
        glyph_matrix[row][col + 1] = ch; // mark trailing cell with same char (occupied)
    }
}

/* Erase symbol at row,col. If the leading char is double-width, erase both halves in one call. */
void erase_symbol(int row, int col, wchar_t **glyph_matrix, int max_width)
{
    if (row < 0 || col < 0 || col >= max_width)
        return;

    wchar_t leading = glyph_matrix[row][col];
    if (leading == L' ' || leading == 0)
    {
        // nothing there; still ensure we clear the cell
        mvaddwstr(row, col, L" ");
        glyph_matrix[row][col] = L' ';
        return;
    }

    int w = wcwidth(leading);
    if (w <= 0)
        w = 1;

    if (w == 2 && col < max_width - 1)
    {
        mvaddwstr(row, col, L"  "); // erase both halves in one call
        glyph_matrix[row][col] = L' ';
        glyph_matrix[row][col + 1] = L' ';
    }
    else
    {
        mvaddwstr(row, col, L" ");
        glyph_matrix[row][col] = L' ';
    }
}

int is_message_column(int message_len, int column, int *message_columns)
{
    for (int i = 0; i < message_len; i++)
    {
        if (column == message_columns[i])
        {
            return 1;
        }
    }
    return 0;
}

wchar_t get_message_char(int message_len, int column, int *message_columns, wchar_t *message)
{
    for (int i = 0; i < message_len; i++)
    {
        if (column == message_columns[i])
        {
            return message[i];
        }
    }
    return L'\0';
}