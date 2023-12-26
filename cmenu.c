#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <sys/ioctl.h>


#define ENTRY_MAX     512
#define PATTERN_MAX   512
#define READ_SIZE     12

#define KEY_DOWN      0x425B1B
#define KEY_UP        0x415B1B
#define KEY_ENTER     0xD
#define KEY_CTRLC     0x03
#define KEY_BACKSPACE 0x7F

#define USAGE_STR     "usage: cmenu <OUTPUT_FILE>"


void error_exit(char*);


char** g_entries       = NULL;
char** g_matches       = NULL;
int    g_entries_len   = 0;
int    g_matches_len   = 0;
int    g_selected      = 0;
int    g_height        = 0;
int    g_terminal      = 0;
bool   g_term_changed  = false;
struct termios g_termios_original;


char* read_stdin(void) {
    char* buf;
    ssize_t size, offset, r;

    size = 0;
    buf = NULL;
    while (1) {
        offset = size;
        size += READ_SIZE;

        if ((buf = realloc(buf, size + 1)) == NULL)
            error_exit("realloc failed");

        r = read(STDIN_FILENO, buf + offset, READ_SIZE);
        if (r != READ_SIZE) {
            if (r == -1)
                error_exit("read failed");

            buf[offset + r] = '\0';
            break;
        }
    };

    return buf;
}


void read_entries(void) {
    char*  left;
    char*  right;
    char** entries;
    size_t i, size;

    left = read_stdin();
    right = left;

    i = size = 0;
    entries = NULL;
    while (*right) {
        if (*right == '\n') {
            *right = '\0';
            if (++i * sizeof(char**) > size) {
                /* just allocate 10 more entries every time */
                size += sizeof(char**) * 10;
                entries = realloc(entries, size);

                if (!entries)
                    error_exit("realloc failed");
            }

            entries[i - 1] = left;
            left = right + 1;
        }

        ++right;
    }

    g_entries     = entries;
    g_matches     = malloc(size);
    g_entries_len = i;

    if (g_matches == NULL)
        error_exit("malloc failed");

    freopen("/dev/tty", "r", stdin);
}


char xtolower(char c) {
    return ((unsigned) c - 'A' < 26) ? c + 'a' - 'A' : c;
}


bool is_match(char* pattern, char* str) {
    if (*pattern == '\0')
        return true;

    /* Pattern could be lowercased just once when reading but that
       makes it display in lowercase too, which I don't want. */
    while ((xtolower(*(pattern++)) == xtolower(*(str++)))) {
        if (*pattern == '\0')
            return true;
        else if (*str == '\0')
            return false;
    }

    return false;
}


void restore_terminal_mode(void) {
    tcsetattr(0, TCSANOW, &g_termios_original);
}


void set_terminal_mode(void) {
    struct termios termios_new;

    tcgetattr(0, &g_termios_original);
    memcpy(&termios_new, &g_termios_original, sizeof termios_new);

    cfmakeraw(&termios_new);
    tcsetattr(0, TCSANOW, &termios_new);
    g_term_changed = true;
}


void termwrite(char* str) {
    int len = strlen(str);

    if (write(g_terminal, str, len) != len)
        error_exit("write failed");
}


void error_exit(char* msg) {
    if (g_term_changed)
        restore_terminal_mode();

    termwrite(msg);
    termwrite("\n");
    exit(EXIT_FAILURE);
}


void clear_screen(void) {
    termwrite("\x1b[H\x1b[2J\x1b[3J");
}


void set_selected_clamped(int n) {
    g_selected =  ((n < 0) ? 0 : n >= g_matches_len ? g_matches_len - 1 : n);
}


void update_matches(char* pattern) {
    int i;

    g_matches_len = 0;
    for (i = 0; i < g_entries_len; ++i) {
        if (is_match(pattern, g_entries[i]))
            g_matches[g_matches_len++] = g_entries[i];
    }
}


int getch(void) {
    char buf[4] = {0};
    return (read(g_terminal, buf, 4) != -1) ? *(int*) buf : -1;
}


void draw(char* pattern) {
    int i, j;

    update_matches(pattern);
    set_selected_clamped(g_selected);
    clear_screen();

    termwrite(">");
    termwrite(pattern);
    termwrite("\n");

    if (g_selected > g_height - 3) {
        i = g_selected - (g_height - 3);
    } else {
        i = 0;
    }

    for (j = 0; i < g_matches_len; ++i) {
        if (++j == g_height - 1)
            break;

        termwrite(g_matches[i]);
        termwrite(i == g_selected ? " (*)\n" : "\n");
    }
}


bool ch_isvalid(char ch) {
    return (ch >= ' ' && ch <= '~');
}


void select_next(void) {
    set_selected_clamped(--g_selected);
}


void select_prev(void) {
    set_selected_clamped(++g_selected);
}


void get_terminal_height(void) {
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    g_height = w.ws_row;
}


int main(void) {
    char  pattern[PATTERN_MAX];
    char* selected_entry;
    int   ch, pattern_len;

    g_terminal = open("/dev/tty", O_RDWR);

    if (g_terminal == -1)
        error_exit("open failed");

    read_entries();
    get_terminal_height();
    draw("");
    set_terminal_mode();

    ch = pattern_len = 0;
    selected_entry = NULL;
    while (1) {
        switch (ch = getch())
        {
        case KEY_CTRLC: goto end;
        case KEY_UP:    select_next(); break;
        case KEY_DOWN:  select_prev(); break;
        case KEY_BACKSPACE:
            if ((pattern_len - 1) > -1)
                --pattern_len;

            pattern[pattern_len] = '\0';
            break;
        case KEY_ENTER:
            if (g_matches_len > 0)
                selected_entry = g_matches[g_selected];

            goto end;

        default:
            if (ch_isvalid(ch)) {
                pattern[pattern_len++] = ch;
                if (pattern_len >= PATTERN_MAX - 1)
                    error_exit("pattern too long");

                pattern[pattern_len] = '\0';
            } else {
                continue;
            }
        }

        restore_terminal_mode();
        draw(pattern_len == 0 ? "" : pattern);
        set_terminal_mode();
    }

end:
    // clear_screen();
    restore_terminal_mode();

    if (selected_entry)
        puts(selected_entry);

    return EXIT_SUCCESS;
}
