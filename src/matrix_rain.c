#include <locale.h>
#include <wchar.h>
#include <ncursesw/ncurses.h>
#include <signal.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#include "ini_parser.h"

typedef enum
{
    COLOR_BRIGHT_GREEN = 8, // First 8 slots are reserved by ncurses
    COLOR_DIMMER_GREEN,
    COLOR_DARK_GREEN
} Color;

typedef enum {
    PAIR_WHITE = 1,
    PAIR_BRIGHT_GREEN,
    PAIR_DIMMER_GREEN,
    PAIR_DARK_GREEN
} ColorPair;

typedef struct
{
    int column;
    int head_row;
    int length;
    int max_length;
    bool active;
} Trail;

typedef struct
{
    wchar_t symbol;
    int color;
} Glyph;

void handle_winch(int sig);
int init_colors();
wchar_t get_random_symbol();
void draw_symbol(int row, int col, wchar_t ch, ColorPair color_pair,
                 Glyph **glyph_matrix, int max_width, int max_height);
void erase_symbol(int row, int col, Glyph **glyph_matrix, int max_width);
int is_message_column(size_t message_len, int column, int *message_columns);
wchar_t get_message_char(size_t message_len, int column, int *message_columns, wchar_t *message);
int would_overwrite_revealed_message(int row, int col, wchar_t ch, int middle_row,
                                     size_t message_len, int *message_columns, bool *message_revealed);

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

    Settings settings;

    if (ini_parse("settings.ini", handler, &settings) < 0) {
        printf("Can't load 'settings.ini'\n");
        return 1;
    }

    // TODO allow any message
    // TODO ensure that message does not exceed screen width
    wchar_t message[] = L"SKINK SYSTEMS";
    size_t message_len = wcslen(message);
    int middle_row = height / 2;
    int message_columns[message_len];
    bool message_revealed[message_len]; // Track which message characters have been revealed
    int frame_counter = 0;
    int last_message_spawn_frame = 0;
    int message_spawn_frame_interval = settings.message_spawn_frame_interval; // Spawn a message trail every n frames (0.5 seconds at 100ms per frame)

    int leftmost_column = (width / 2) - (message_len / 2);

    for (int i = 0; i < message_len; i++)
    {
        message_columns[i] = leftmost_column + i;
        message_revealed[i] = false; // Initially no characters are revealed
    }

    // dynamically allocate glyph_matrix
    Glyph **glyph_matrix = malloc(height * sizeof(Glyph *));
    if (!glyph_matrix)
    {
        endwin();
        return 1;
    }
    for (int i = 0; i < height; i++)
    {
        glyph_matrix[i] = malloc(width * sizeof(Glyph));
        if (!glyph_matrix[i])
        {
            for (int k = 0; k < i; k++)
                free(glyph_matrix[k]);
            free(glyph_matrix);
            endwin();
            return 1;
        }
        for (int j = 0; j < width; j++)
            glyph_matrix[i][j].symbol = L' ';
    }

    size_t max_trails = width + width * (height / settings.max_trail_length);
    Trail trails[max_trails];
    size_t num_trails = 0;

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
                            message_revealed[j] = true;
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
                    // Check if this character would overwrite revealed message characters
                    if (!would_overwrite_revealed_message(head_row, column, ch, middle_row,
                                                          message_len, message_columns, message_revealed))
                    {
                        draw_symbol(head_row, column, ch, PAIR_WHITE, glyph_matrix, width, height);
                    }
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
                            is_revealed = true;
                            break;
                        }
                    }
                    if (!is_revealed)
                    {
                        // Not revealed yet, draw normal trail character
                        wchar_t ch = glyph_matrix[r][column].symbol;
                        draw_symbol(r, column, ch, PAIR_BRIGHT_GREEN, glyph_matrix, width, height);
                    }
                }
                else
                {
                    wchar_t ch = glyph_matrix[r][column].symbol;
                    // Check if this character would overwrite revealed message characters
                    if (!would_overwrite_revealed_message(r, column, ch, middle_row,
                                                          message_len, message_columns, message_revealed))
                    {
                        draw_symbol(r, column, ch, PAIR_BRIGHT_GREEN, glyph_matrix, width, height);
                    }
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
                            is_revealed = true;
                            break;
                        }
                    }
                    if (!is_revealed)
                    {
                        // Not revealed yet, draw normal trail character
                        wchar_t ch = glyph_matrix[r][column].symbol;
                        draw_symbol(r, column, ch, PAIR_DIMMER_GREEN, glyph_matrix, width, height);
                    }
                }
                else
                {
                    wchar_t ch = glyph_matrix[r][column].symbol;
                    // Check if this character would overwrite revealed message characters
                    if (!would_overwrite_revealed_message(r, column, ch, middle_row,
                                                          message_len, message_columns, message_revealed))
                    {
                        draw_symbol(r, column, ch, PAIR_DIMMER_GREEN, glyph_matrix, width, height);
                    }
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
                            is_revealed = true;
                            break;
                        }
                    }
                    if (!is_revealed)
                    {
                        // Not revealed yet, draw normal trail character
                        wchar_t ch = glyph_matrix[r][column].symbol;
                        draw_symbol(r, column, ch, PAIR_DARK_GREEN, glyph_matrix, width, height);
                    }
                }
                else
                {
                    wchar_t ch = glyph_matrix[r][column].symbol;
                    // Check if this character would overwrite revealed message characters
                    if (!would_overwrite_revealed_message(r, column, ch, middle_row,
                                                          message_len, message_columns, message_revealed))
                    {
                        draw_symbol(r, column, ch, PAIR_DARK_GREEN, glyph_matrix, width, height);
                    }
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
                            is_revealed = true;
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

            // ...inside the main trail loop, after head_row > 0 check...

                        if (tail_row >= height)
            {
                current->active = false;
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

        // Smart trail spawning - prioritize unrevealed message columns
        frame_counter++;
        int should_spawn_message_trail = (frame_counter - last_message_spawn_frame) >= message_spawn_frame_interval;

        // Add new trail if space available
        if (num_trails < max_trails)
        {
            int spawned = 0;

            // First priority: spawn a trail in an unrevealed message column if it's time
            if (should_spawn_message_trail)
            {
                for (int msg_idx = 0; msg_idx < message_len; msg_idx++)
                {
                    if (!message_revealed[msg_idx])
                    {
                        int msg_col = message_columns[msg_idx];
                        // Check if this column is available for a new trail
                        int left_ok = (glyph_matrix[0][msg_col].symbol == L' ');
                        int left_left_ok = (msg_col > 0) ? (glyph_matrix[0][msg_col - 1].symbol == L' ') : 1;
                        int right_ok = (msg_col < width - 1) ? (glyph_matrix[0][msg_col + 1].symbol == L' ') : 1;

                        if (left_ok && left_left_ok && right_ok)
                        {
                            // Find an inactive trail to use
                            for (int i = 0; i < max_trails; i++)
                            {
                                if (!trails[i].active)
                                {
                                    trails[i].column = msg_col;
                                    trails[i].head_row = 0;
                                    trails[i].length = settings.max_trail_length;
                                    trails[i].max_length = settings.max_trail_length;
                                    trails[i].active = true;
                                    num_trails++;
                                    spawned = 1;
                                    last_message_spawn_frame = frame_counter;
                                    break;
                                }
                            }
                            break; // Only spawn one message trail at a time
                        }
                    }
                }
            }

            // Second priority: spawn regular random trails if we didn't spawn a message trail
            if (!spawned)
            {
                for (int i = 0; i < max_trails; i++)
                {
                    if (!trails[i].active)
                    {
                        int random_column = rand() % width;

                        // avoid starting in or next to an occupied cell (also treats right-half as occupied)
                        int left_ok = (glyph_matrix[0][random_column].symbol == L' ');
                        int left_left_ok = (random_column > 0) ? (glyph_matrix[0][random_column - 1].symbol == L' ') : 1;
                        int right_ok = (random_column < width - 1) ? (glyph_matrix[0][random_column + 1].symbol == L' ') : 1;

                        if (!left_ok || !left_left_ok || !right_ok)
                        {
                            continue;
                        }

                        trails[i].column = random_column;
                        trails[i].head_row = 0;
                        trails[i].length = settings.max_trail_length;
                        trails[i].max_length = settings.max_trail_length;
                        trails[i].active = 1;
                        num_trails++;
                        break;
                    }
                }
            }
        }

        refresh();
        napms(settings.refresh_rate);
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
 * Also checks if drawing a wide char would overwrite revealed message characters.
 */
void draw_symbol(int row, int col, wchar_t ch, ColorPair color_pair,
                 Glyph **glyph_matrix, int max_width, int max_height)
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
    glyph_matrix[row][col].symbol = ch;
    glyph_matrix[row][col].color = color_pair;
    if (w == 2 && col + 1 < max_width)
    {
        glyph_matrix[row][col + 1].symbol = ch; // mark trailing cell with same char (occupied)
        glyph_matrix[row][col + 1].color = color_pair;
    }
}

/* Erase symbol at row,col. If the leading char is double-width, erase both halves in one call. */
void erase_symbol(int row, int col, Glyph **glyph_matrix, int max_width)
{
    if (row < 0 || col < 0 || col >= max_width)
        return;

    wchar_t leading = glyph_matrix[row][col].symbol;
    if (leading == L' ' || leading == 0)
    {
        // nothing there; still ensure we clear the cell
        mvaddwstr(row, col, L" ");
        glyph_matrix[row][col].symbol = L' ';
        return;
    }

    int w = wcwidth(leading);
    if (w <= 0)
        w = 1;

    if (w == 2 && col < max_width - 1)
    {
        mvaddwstr(row, col, L"  "); // erase both halves in one call
        glyph_matrix[row][col].symbol = L' ';
        glyph_matrix[row][col + 1].symbol = L' ';
    }
    else
    {
        mvaddwstr(row, col, L" ");
        glyph_matrix[row][col].symbol = L' ';
    }
}

int is_message_column(size_t message_len, int column, int *message_columns)
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

wchar_t get_message_char(size_t message_len, int column, int *message_columns, wchar_t *message)
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

/* Check if drawing a character at row,col would overwrite a revealed message character.
 * This considers that wide characters (wcwidth=2) occupy two columns.
 */
int would_overwrite_revealed_message(int row, int col, wchar_t ch, int middle_row,
                                     size_t message_len, int *message_columns, bool *message_revealed)
{
    if (row != middle_row)
        return 0; // Not at message row

    int w = wcwidth(ch);
    if (w <= 0)
        w = 1;

    // Check if this character or its wide extension would overwrite a revealed message char
    for (int offset = 0; offset < w; offset++)
    {
        int check_col = col + offset;
        for (int i = 0; i < message_len; i++)
        {
            if (message_columns[i] == check_col && message_revealed[i])
            {
                return 1; // Would overwrite a revealed message character
            }
        }
    }

    return 0; // Safe to draw
}