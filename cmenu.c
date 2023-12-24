#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>


#define ENTRY_MAX     512
#define ENTRIES_MAX   512
#define PATTERN_MAX   512

#define KEY_DOWN      0x425B1B
#define KEY_UP        0x415B1B
#define KEY_ENTER     0xD
#define KEY_CTRLC     0x03
#define KEY_BACKSPACE 0x7F
#define USAGE_STR     "usage: cmenu <OUTPUT_FILE> [-r|--remove]"

#define ERROR_EXIT(msg) \
    do { puts(msg); exit(EXIT_FAILURE); } while (0)


void restore_terminal_mode(void);


char** g_entries       = NULL;
char** g_matches       = NULL;
int    g_entries_len   = 0;
int    g_matches_len   = 0;
int    g_selected      = 0;
int    g_height        = 0;
struct termios g_termios_original;


void clear_screen(void) {
    system("clear");
}


int read_entry(char** dest) {
    char buf[ENTRY_MAX];
    char ch;
    int  i;

    i = 0;
    *dest = NULL;
    while ((ch = fgetc(stdin)) != EOF) {
        if (ch != '\n' && i < (ENTRY_MAX - 1)) {
            buf[i++] = ch;
        } else {
            break;
        }
    }

    if (i > 0) {
        buf[i] = '\0';
        *dest = strdup(buf);
    }

    return ch == EOF ? 1 : 0;
}


bool is_match(char* pattern, char* str) {
    if (*pattern == '\0')
        return true;

    while ((tolower(*(pattern++)) == tolower(*(str++)))) {
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
}


bool key_pressed(void) {
    fd_set fds;
    struct timeval tv;

    tv.tv_sec  = 0;
    tv.tv_usec = 0;

    FD_ZERO(&fds);
    FD_SET(0, &fds);

    return select(1, &fds, NULL, NULL, &tv) > 0;
}


void read_entries(void) {
    char* entries[ENTRIES_MAX];
    char* entry;
    int   i, size, code;

    i = 0;
    while (1) {
        code = read_entry(&entry);

        if (i > ENTRIES_MAX - 1)
            ERROR_EXIT("Too many entries.");

        if (entry) {
            entries[i++] = entry;
            size += sizeof(char**);
        }

        if (code)
            break;
    }

    g_entries     = malloc(size);
    g_matches     = malloc(size);
    g_entries_len = i;

    if ( !(g_entries && g_matches) )
        ERROR_EXIT("malloc failed");

    memcpy(g_entries, entries, size);
    freopen("/dev/tty", "r", stdin);
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
    return (read(0, buf, 4) != -1) ? *(int*) buf : -1;
}


void draw(char* pattern) {
    int i, j;

    update_matches(pattern);
    set_selected_clamped(g_selected);
    clear_screen();

    printf(">%s\n", pattern);

    if (g_selected > g_height - 3) {
        i = g_selected - (g_height - 3);
    } else {
        i = 0;
    }

    for (j = 0; i < g_matches_len; ++i) {
        if (++j == g_height - 1)
            break;

        printf("%s%s\n", g_matches[i], i == g_selected ? " (*)" : "");
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


int main(int argc, char** argv) {
    FILE* fp;
    char  pattern[PATTERN_MAX];
    char* selected_entry;
    int   ch, pattern_len;

    switch (argc)
    {
    case 2:
        break;

    case 3:
        if (strcmp(argv[2], "-r") == 0 || strcmp(argv[2], "--remove") == 0) {
            if (access(argv[1], F_OK) == -1)
                ERROR_EXIT("file does not exist");

            if ( (fp = fopen(argv[1], "r")) ) {
                while ((ch = fgetc(fp)) != EOF)
                    putchar(ch);

                fclose(fp);
            }

            unlink(argv[1]);
            return EXIT_SUCCESS;
        } else {
            ERROR_EXIT(USAGE_STR);
        }

    default:
        ERROR_EXIT(USAGE_STR);
    }

    read_entries();
    get_terminal_height();
    draw("");
    set_terminal_mode();

    ch = pattern_len = 0;
    selected_entry = NULL;
    while (1) {
        if (key_pressed()) {
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
                        ERROR_EXIT("pattern too long");

                    pattern[pattern_len] = '\0';
                } else {
                    continue;
                }
            }

            restore_terminal_mode();
            draw(pattern_len == 0 ? "" : pattern);
            set_terminal_mode();
        }
    }

end:
    if (selected_entry) {
        if (access(argv[1], F_OK) != -1)
            ERROR_EXIT("file already exists");

        if ( (fp = fopen(argv[1], "w+")) ) {
            fputs(selected_entry, fp);
            fclose(fp);
        }
    }

    restore_terminal_mode();
    return EXIT_SUCCESS;
}
