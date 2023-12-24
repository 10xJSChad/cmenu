#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>
#include <time.h>


#define ENTRY_MAX   512
#define ENTRIES_MAX 512
#define PATTERN_MAX 512

#define KEY_DOWN      0x425B1B
#define KEY_UP        0x415B1B
#define KEY_ENTER     0xD
#define KEY_CTRLC     0x03
#define KEY_BACKSPACE 0x7F
#define USAGE_STR "usage: cmenu <OUTPUT_FILE> [-r|--remove]"


void exit_with(char* msg) {
    puts(msg);
    exit(1);
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
            buf[i] = '\0';
            *dest = strdup(buf);
            return 0;
        }
    }

    return 1;
}


void clear_screen(void) {
    system("clear");
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


char** g_entries       = NULL;
char** g_matches       = NULL;
int    g_entries_len   = 0;
int    g_matches_len   = 0;
int    g_selected      = 0;
struct termios g_termios_original;


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


int key_pressed(void) {
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
    int   i, size;

    i = 0;
    while ( (read_entry(&entry) == 0) ) {
        if (i > ENTRIES_MAX - 1)
            exit_with("Too many entries.");

        if (entry) {
            entries[i++] = entry;
            size += sizeof(char**);
        }
    }

    g_entries     = malloc(size);
    g_matches     = malloc(size);
    g_entries_len = i;

    if ( !(g_entries && g_matches) )
        exit_with("malloc failed");

    memcpy(g_entries, entries, size);
    freopen("/dev/tty", "r", stdin);
}


int clamp(int n) {
    return (n < 0) ? 0 : n >= g_matches_len ? g_matches_len - 1 : n;
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
    int i;

    update_matches(pattern);
    g_selected = clamp(g_selected);
    clear_screen();

    printf(">%s\n", pattern);
    for (i = 0; i < g_matches_len; ++i) {
        printf("%s%s\n", g_matches[i], i == g_selected ? " (*)" : "");
    }
}


bool ch_isvalid(char ch) {
    return (ch >= ' ' && ch <= '~');
}


#define USAGE_STR "usage: cmenu <OUTPUT_FILE> [-r|--remove]"


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
                exit_with("file does not exist");

            if ( (fp = fopen(argv[1], "r")) ) {
                while ((ch = fgetc(fp)) != EOF)
                    putchar(ch);

                fclose(fp);
            }

            unlink(argv[1]);
            return 0;
        } else {
            exit_with(USAGE_STR);
        }

    default:
        exit_with(USAGE_STR);
    }

    read_entries();
    draw("");
    set_terminal_mode();

    ch = pattern_len = 0;
    selected_entry = NULL;
    while (1) {
        if (key_pressed()) {
            switch (ch = getch())
            {
            case KEY_UP:    g_selected = clamp(--g_selected); break;
            case KEY_DOWN:  g_selected = clamp(++g_selected); break;
            case KEY_CTRLC: goto end;
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
                        exit_with("pattern too long");

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
            exit_with("file already exists");

        if ( (fp = fopen(argv[1], "w+")) ) {
            fputs(selected_entry, fp);
            fclose(fp);
        }
    }

    restore_terminal_mode();
    return 0;
}