/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h> /* This is a test of
multiline comment */ 
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdint.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

/*** defines ***/

#define EDITOR_QUIT_TIMES 1 
#define EDITOR_TAB_STOP 8
#define EDITOR_VERSION "0.0.1"

#define CTRL_KEY(k) ((k) & 0x1f)

#define DIGIT_COLOR "\x1b[31m"
#define COLORING_STOP_BYTE "\x1b[39m"

enum arrowsKeys {
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DELETE_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

enum highlighting {
    HL_NORMAL = 0,
    HL_COMMENT,
    HL_MLCOMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH
};

#define HL_HIGHLIGHT_NUMBERS (1 << 0)
#define HL_HIGHLIGHT_STRINGS (1 << 1)

/*** data ***/

struct editorSyntax {
    char *filetype;
    char **filematch;
    char **keywords;
    char *singleline_comment_start;
    char *multiline_comment_start;
    char *multiline_comment_end;
    int flags;
};

typedef struct erow {
    int idx;
    int size;
    int rendersize;
    char *chars;
    char *render;
    char *highlight;
    int hl_open_comment;
} erow;

struct config {
    int cx, cy;                         //cursor coordinates (cx - row, cy - column)
    int rx;                             //render string horizontal coordinate
    int coloff;
    int rowoff;
    int screenrows;
    int screencols;
    int rowsnum;
    erow *row;                          //file rows
    int dirty;
    char *filename; 
    char statusmsg[80];
    time_t statusmsg_time;
    struct editorSyntax *syntax;
    struct termios original_termios;    //previous terminal state (for recover after exiting editor)
};

struct config E;

/*** filetypes ***/

char *C_HIGHLIGHT_FILETYPES[] = {".c", ".h", ".cpp", ".hpp", NULL};
char *C_HIGHLIGHT_KEYWORDS[] = {
    "switch", "if", "while", "for", "break", "default", "return", 
    "else", "struct", "union", "typedef", "enum", "class", "case", 
    "const|", "constexpr|", "consteval|", "constinit|", "concept|",
    "template|", "requires", "asm", "int|", "char|", "short|",
    "long|", "float|", "double|", "unsigned|", "signed|", NULL
};

struct editorSyntax HIGHLIGHT_DB[] = {
     {
        "c",
        C_HIGHLIGHT_FILETYPES,
        C_HIGHLIGHT_KEYWORDS,
        "//", "/*", "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
     },
};

#define HIGHLIGHT_DB_ENTRIES (sizeof(HIGHLIGHT_DB) / sizeof(HIGHLIGHT_DB[0]))

/*** prototypes ***/

void setStatusMessage(const char *fmt, ...);
void refreshScreen(void);
char* prompt(char *prompt, void (*callback)(const char*, const int));

/*** terminal ***/

void die(const char* s) //if error occured, this function appears
{   
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(EXIT_FAILURE);
}

void disableRawMode(void)
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.original_termios) == -1) {
        die("tcsetattr");
    }
}

void enableRawMode(void)
{
    if (tcgetattr(STDIN_FILENO, &E.original_termios) == -1)  {
        die("tcsetattr");
    }

    atexit(disableRawMode);

    struct termios raw_termios = E.original_termios;
    raw_termios.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw_termios.c_oflag &= ~(OPOST);
    raw_termios.c_cflag |= CS8;
    raw_termios.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw_termios.c_cc[VMIN] = 0;
    raw_termios.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios) == -1) {
        die("tcsetattr");
    }
}

int readKey(void)
{
    int nread;
    char c;
    
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)    {
        if (nread == -1 && errno != EAGAIN) die("read");
    }

    if (c == '\x1b')    {
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[')   {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~')  {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DELETE_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            }
            else {
            switch (seq[1]) {
                case 'A': return ARROW_UP;
                case 'B': return ARROW_DOWN;
                case 'C': return ARROW_RIGHT;
                case 'D': return ARROW_LEFT;
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
          }
        } else if (seq[0] == 'O')   {
            switch (seq[1]) {
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }

        return '\x1b';
    } else {
        return c;
    }
}

int getCursorPosition(int *rows, int *cols)
{
    char buf[32];
    unsigned int i = 0u;
    
    if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
   
    while (i < sizeof(buf) - 1) {
        if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
        if (buf[i] == 'R') break;
        ++i;
    }

    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;

    return 0;
}
 
short getWindowSize(int *rows, int *cols)
{
    struct winsize ws;

    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)   {
            return -1;
        }
        return getCursorPosition(rows, cols);
    }
        else {
            *cols = ws.ws_col;
            *rows = ws.ws_row;
            return 0;
    }
}

/*** syntax highlighting ***/

int is_separator(int c)
{
    return isspace(c) || c == '\0' || strchr(",.()+-/*=~%<>[];", c) != NULL;
}

void updateSyntax(erow *row)
{
    row->highlight = realloc(row->highlight, row->rendersize);
    memset(row->highlight, HL_NORMAL, row->rendersize);
    
    if (E.syntax == NULL) return;
    
    char **keywords = E.syntax->keywords;

    const char *scs = E.syntax->singleline_comment_start;
    const char *mcs = E.syntax->multiline_comment_start;
    const char *mce = E.syntax->multiline_comment_end;

    const uint32_t scs_len = scs ? strlen(scs) : 0;
    const uint32_t mcs_len = mcs ? strlen(mcs) : 0;
    const uint32_t mce_len = mce ? strlen(mcs) : 0;

    int prev_separator = 1, in_string = 0, in_comment = (row->idx > 0 && E.row[row->idx - 1].hl_open_comment);
    int i = 0;

    while (i < row->rendersize) {
        const char symbol = row->render[i];
        unsigned char prev_highlight = (i > 0) ? row->highlight[i - 1] : HL_NORMAL;

        if (scs_len && !in_string && !in_comment)  {
            if (!strncmp(&row->render[i], scs, scs_len))    {
                memset(&row->highlight[i], HL_COMMENT, row->rendersize - i);
                break;
            }
        }

        if (mcs_len && mce_len && !in_string)   {
            if (in_comment) {
                row->highlight[i] = HL_MLCOMMENT;
                if (!strncmp(&row->render[i], mce, mce_len))    {
                    memset(&row->highlight[i], HL_MLCOMMENT, mce_len);
                    i += mce_len;
                    in_comment = 0;
                    prev_separator = 1;
                    continue;
                } else {
                    i++;
                    continue;
                }
            } else if (!strncmp(&row->render[i], mcs, mcs_len)) {
                memset(&row->highlight[i], HL_MLCOMMENT, mcs_len);
                i += mcs_len;
                in_comment = 1;
                continue;
            }
        }

        if (E.syntax->flags && HL_HIGHLIGHT_STRINGS)    {
            if (in_string)  {
                row->highlight[i] = HL_STRING;

                if (symbol == '\\' && i + 1 < row->rendersize)   {
                    row->highlight[i + 1] = HL_STRING;
                    i += 2;
                    continue;
                }

                if (symbol == in_string) in_string = 0;

                prev_separator = 1;
                i++;
                continue;
            } else {
                if (symbol == '"' || symbol == '\\')   {
                    in_string = symbol;
                    row->highlight[i] = HL_STRING;
                    i++;
                    continue;
                }
            }
        }

        if (E.syntax->flags && HL_HIGHLIGHT_NUMBERS) {
            if ((isdigit(symbol) && (prev_separator || prev_highlight == HL_NUMBER)) || (symbol == '.' && prev_highlight == HL_NUMBER))   {
                row->highlight[i] = HL_NUMBER;
                prev_separator = 0;
                i++;
                continue;
            }
        }

        if (prev_separator) {
            uint16_t j = 0;

            for (; keywords[j]; ++j)  {
                int32_t keyword_len = strlen(keywords[j]);
                const int32_t is_keyword2 = keywords[j][keyword_len - 1] == '|';

                if (is_keyword2) keyword_len--;

                if (!strncmp(&row->render[i], keywords[j], keyword_len) && is_separator(row->render[i + keyword_len]))  {
                    memset(&row->highlight[i], is_keyword2 ? HL_KEYWORD2 : HL_KEYWORD1, keyword_len);
                    i += keyword_len;
                    break;
                }
            }

            if (keywords[j] != NULL)    {
                prev_separator = 0;
                continue;
            }
        }

        prev_separator = is_separator(symbol);
        i++;
    }

    int changed = (row->hl_open_comment != in_comment);
    row->hl_open_comment = in_comment;
    if (changed && row->idx + 1 < E.rowsnum)
        updateSyntax(&E.row[row->idx + 1]);
}

int mapSyntaxToColor(const int highlight)
{
    switch (highlight)  {
        case HL_COMMENT:
        case HL_MLCOMMENT: return 36;
        case HL_KEYWORD1: return 33;
        case HL_KEYWORD2: return 32;
        case HL_NUMBER: return 31; //TODO: 8-bit colors!!!
        case HL_MATCH: return 34;
        case HL_STRING: return 35;
        default: return 37;
    }
}

void selectSyntaxHighlight()
{
    E.syntax = NULL;
    if (E.filename == NULL) return;

    char *filetype = strchr(E.filename, '.');

    for (uint32_t j = 0; j < HIGHLIGHT_DB_ENTRIES; ++j) {
        struct editorSyntax *s = &HIGHLIGHT_DB[j];
        uint32_t i = 0;

        while (s->filematch[i]) {
            int is_filetype = (s->filematch[i][0] == '.');
            if ((is_filetype && filetype && !strcmp(filetype, s->filematch[i])) || (!is_filetype && strstr(E.filename, s->filematch[i])))   {
                E.syntax = s;

                for (int32_t filerow = 0; filerow < E.rowsnum; ++filerow)   {
                    updateSyntax(&E.row[filerow]);
                }

                return;
            }

            i++;
        }
    }
}

/*** row operations ***/

int convertCxToRx(erow* row, const int cx)
{
    int rx = 0;
    for (int j = 0; j < cx; ++j)    {
        if (row->chars[j] == '\t')  {
            rx += (EDITOR_TAB_STOP - 1) - (rx % EDITOR_TAB_STOP);
        }
        rx++;
    }

    return rx;
}

int convertRxToCx(erow *row, const int rx)
{
    int current_rx = 0, cx;
    for (cx = 0; cx < row->size; ++cx)  {
        if (row->chars[cx] == '\t')
            current_rx += (EDITOR_TAB_STOP - 1) - (current_rx % EDITOR_TAB_STOP);
        current_rx++;

        if (current_rx > rx) return cx;
    }

    return cx;
}

void updateRow(erow *row)
{
    int tabs = 0;
    int j;
    for (j = 0; j < row->size; ++j) {
        if (row->chars[j] == '\t') ++tabs;
    }

    free(row->render);
    row->render = malloc(row->size + tabs * (EDITOR_TAB_STOP - 1) + 1);

    int idx = 0;
    for (j = 0; j < row->size; ++j) {
        if (row->chars[j] == '\t')  {
            row->render[idx++] = ' ';
            while (idx % EDITOR_TAB_STOP != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
    }
        }
    row->render[idx] = '\0';
    row->rendersize = idx;

    updateSyntax(row);
}

void insertRow(const int index, const char *s, const size_t len) 
{
    if (index < 0 || index > E.rowsnum) return;

    E.row = realloc(E.row, sizeof(erow) * (E.rowsnum + 1));
    memmove(&E.row[index + 1], &E.row[index], sizeof(erow) * (E.rowsnum - index));
    
    for (int j = index + 1; j <= E.rowsnum; ++j)   E.row[j].idx++;

    E.row[index].idx = index;

    E.row[index].size = len;
    E.row[index].chars = malloc(len + 1);
    memcpy(E.row[index].chars, s, len);
    E.row[index].chars[len] = '\0';

    E.row[index].rendersize = 0;
    E.row[index].render = NULL;
    E.row[index].highlight = NULL;
    E.row[index].hl_open_comment = 0;
    updateRow(&E.row[index]);

    E.rowsnum++;
    E.dirty++;
}

void freeRow(erow *row)
{
    free(row->render);
    free(row->chars);
    free(row->highlight);
}

void deleteRow(const int index)
{
    if (index < 0 || index >= E.rowsnum)  return;
    freeRow(&E.row[index]);
    memmove(&E.row[index], &E.row[index + 1], sizeof(erow) * (E.rowsnum - index - 1));
    
    for (int j = index; j < E.rowsnum - 1; ++j) E.row[j].idx--;

    E.rowsnum--;
    E.dirty++;
}

void insertCharInRow(erow *row, int index, const int character)
{
    if (index < 0 || index > row->size) index = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[index + 1], &row->chars[index], row->size - index + 1);
    row->size++;
    row->chars[index] = character;
    updateRow(row);
    E.dirty++;
}

void appendStringInRow(erow *row, const char *str, const size_t len)
{
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], str, len);
    row->size += len;
    row->chars[row->size] = '\0';
    updateRow(row);
    E.dirty++;
}

void deleteCharFromRow(erow *row, const int index)
{
    if (index < 0 || index >= row->size) return;
    memmove(&row->chars[index], &row->chars[index + 1], row->size - index);
    row->size--;
    updateRow(row);
    E.dirty++;
}

/*** editor operations ***/

void insertChar(const int character)
{
    if (E.cy == E.rowsnum)   {
        insertRow( E.rowsnum, "", 0);
    }

    insertCharInRow(&E.row[E.cy], E.cx, character);
    E.cx++;
}

void insertNewLine() // "Enter" keypress
{
    if (E.cx == 0)  {
        insertRow(E.cy, "", 0);
    } else {
        erow *row = &E.row[E.cy];
        insertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
        row = &E.row[E.cy];
        row->size = E.cx;
        row->chars[row->size] = '\0';
        updateRow(row);
    }

    E.cy++;
    E.cx = 0;
}

void deleteChar()
{   
    if (E.cy == E.rowsnum || (E.cx == 0 && E.cy == 0)) return;

    erow *row = &E.row[E.cy];
    if (E.cx > 0)   {
        deleteCharFromRow(row, E.cx - 1);
        E.cx--;
    } else {
        E.cx = E.row[E.cy - 1].size;
        appendStringInRow(&E.row[E.cy - 1], row->chars, row->size);
        deleteRow(E.cy);
        E.cy--;
    }
}

/*** file io ***/

char* convertRowsToSingleString(int *buflen)
{
    int totallen = 0, j;
    for (j = 0; j < E.rowsnum; ++j) {
        totallen += E.row[j].size + 1;
    }
    *buflen = totallen;

    char *buf = malloc(totallen);
    char *p = buf;
    for (j = 0; j < E.rowsnum; ++j) {
        memcpy(p, E.row[j].chars, E.row[j].size);
        p += E.row[j].size;
        *p = '\n';
        p++;
    }

    return buf;
}

void openFile(const char *filename)
{   
    free(E.filename);
    E.filename = strdup(filename);

    selectSyntaxHighlight();

    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, fp)) != -1)  {
        while ((linelen > 0 && (line[linelen - 1] == '\n')) || (line[linelen - 1] == '\r')) {
            linelen--;
        }
        insertRow(E.rowsnum, line, linelen);
    }

    free(line);
    fclose(fp);
    E.dirty = 0;
}

void saveFile()
{
    if (E.filename == NULL) {
        E.filename = prompt("Save as: %s (ESC to cancel)", NULL);
        if (E.filename == NULL) {
            setStatusMessage("Save aborted!");
            return;
        }

        selectSyntaxHighlight();
    }

    int len;
    char *buf = convertRowsToSingleString(&len);

    const int fd = open(E.filename, O_RDWR | O_CREAT, 0644);
    if (fd != -1)   {
        if (ftruncate(fd, len) != -1)   {
           if (write(fd, buf, len) == len)  {
            close(fd);
            free(buf);
            E.dirty = 0;
            setStatusMessage("%d bytes written to disk", len);
            return;
           }
        }

        close(fd);
    }

    free(buf);
    setStatusMessage("I/O Error: %s", strerror(errno));
}

/*** search ***/

void searchCallback(const char *query, const int key)
{
    static int last_match = -1;
    static int direction = 1;

    static int saved_highlight_line;
    static char *saved_highlight = NULL;

    if (saved_highlight)    {
        memcpy(E.row[saved_highlight_line].highlight, saved_highlight, E.row[saved_highlight_line].rendersize);
        free(saved_highlight);
        saved_highlight = NULL;
    }

    if (key == '\r' || key == '\x1b')   {
        last_match = -1;
        direction = 1;
        return;
    } else if (key == ARROW_RIGHT || key == ARROW_DOWN) {
        direction = 1;
    } else if (key == ARROW_LEFT || key == ARROW_UP)    {
        direction = -1;
    } else {
        last_match = -1;
        direction = 1;
    }

    if (last_match == -1) direction = 1;
    int current = last_match;

    for (int i = 0; i < E.rowsnum; ++i) {
        current += direction;
        if (current == -1) current = E.rowsnum - 1;
        else if (current == E.rowsnum) current = 0;

        erow *row = &E.row[current];
        char *match = strstr(row->render, query);
        if (match)  {
            last_match = current;
            E.cy = current;
            E.cx = convertRxToCx(row, match - row->render);
            E.rowoff = E.rowsnum;

            saved_highlight_line = current;
            saved_highlight = malloc(row->rendersize);
            memcpy(saved_highlight, row->highlight, row->rendersize);
            memset(&row->highlight[match - row->render], HL_MATCH, strlen(query));
            break;
        }
    }
}

void search()
{
    int saved_cx = E.cx, saved_cy = E.cy;
    int saved_coloff = E.coloff, saved_rowoff = E.rowoff;

    char *query = prompt("Search: %s (Use ESC/Arrows/Enter)", searchCallback);
    if (query) free(query);
    else {
        E.cx = saved_cx;
        E.cy = saved_cy;
        E.coloff = saved_coloff;
        E.rowoff = saved_rowoff;
    }
}

/*** append buffer ***/

struct abuf {
    char* buf;
    unsigned int len;
};

#define ABUF_INIT {NULL, 0}

void abufAppend(struct abuf *ab, const char *s, unsigned int len)   
{
    char* new = realloc(ab->buf, ab->len + len);

    if (new == NULL) return;
    memcpy(&new[ab->len], s, len);
    ab->buf = new;
    ab->len += len;
}

void abufFree(struct abuf *ab) 
{
    free(ab->buf);
}

/*** output ***/

void scroll()   
{
    E.rx = 0;
    if (E.cy < E.rowsnum)   {
        E.rx = convertCxToRx(&E.row[E.cy], E.cx);
    }

    if (E.cy < E.rowoff)    {
        E.rowoff = E.cy;
    }

    if (E.cy >= E.rowoff + E.screenrows)    {
        E.rowoff = E.cy - E.screenrows + 1;
    }

    if (E.rx < E.coloff)    {
        E.coloff = E.rx;
    }

    if (E.rx >= E.coloff + E.screencols)    {
        E.coloff = E.rx - E.screencols + 1;
    }
}

void drawRows(struct abuf *ab)
{
   int y;
   for (y = 0; y < E.screenrows; ++y)  {
        const int filerow = y + E.rowoff;

        if (filerow >= E.rowsnum) {
            if (E.rowsnum == 0 && y == E.screenrows / 3)   {
                char msg[80];
                int msglen = snprintf(msg, sizeof(msg), "This is my pet-project text-editor! Version %s", EDITOR_VERSION);

                if (msglen > E.screencols) 
                    msglen = E.screencols;

                int padding = (E.screencols - msglen) / 2;

                if (padding)    {
                    abufAppend(ab, "~", 1);
                    --padding;
                }

                while (padding--)  abufAppend(ab, " ", 1);

                abufAppend(ab, msg, msglen);
            } else {
                abufAppend(ab, "~", 1);
            }
        } else {
            int len = E.row[filerow].rendersize - E.coloff;
            if (len < 0) len = 0;
            if (len > E.screencols) len = E.screencols;
            char *c = &E.row[filerow].render[E.coloff]; //TODO: this should be put into a separate procedure
            char *highlight = &E.row[filerow].highlight[E.coloff];
            int current_color = -1;

            for (int j = 0; j < len; ++j)   {
                if (iscntrl(c[j]))  {
                    const char symbol = (c[j] <= 26) ? '@' + c[j] : '?';
                    abufAppend(ab, "\x1b[7m", 4);
                    abufAppend(ab, &symbol, 1);
                    abufAppend(ab, "\x1b[m", 3);
                    if (current_color != -1) {
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", current_color);
                        abufAppend(ab, buf, clen);
                    }
                } else if (highlight[j] == HL_NORMAL)  {
                    abufAppend(ab, COLORING_STOP_BYTE, 5);
                    abufAppend(ab, &c[j], 1);
                } else {
                    int color = mapSyntaxToColor(highlight[j]);
                    char buf[16];
                    int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color); //COLOR length
                    abufAppend(ab, buf, clen);
                    abufAppend(ab, &c[j], 1);
                }
            }

            abufAppend(ab, COLORING_STOP_BYTE, 5);
        }

    abufAppend(ab, "\x1b[K", 3);
    abufAppend(ab, "\r\n", 2);
  }
}

void drawStatusBar(struct abuf *buf)    
{
    abufAppend(buf, "\x1b[7m", 4);
    char status[80], rstatus[80];

    int len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
        E.filename ? E.filename : "[NO NAME]", E.rowsnum, E.dirty ? "(modified)" : "");

    int rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d", E.syntax ? E.syntax->filetype : "no filetype", E.cy + 1, E.rowsnum);

    if (len > E.screencols) len = E.screencols;
    abufAppend(buf, status, len);

    while (len < E.screencols)  {
        if (E.screencols - len == rlen) {
            abufAppend(buf, rstatus, rlen);
            break;
        }

        abufAppend(buf, " ", 1);
        len++;
    }

    abufAppend(buf, "\x1b[m", 3);
    abufAppend(buf, "\r\n", 2);
}

void drawMessageBar(struct abuf *buf)
{
    abufAppend(buf, "\x1b[K", 3);
    int msglen = strlen(E.statusmsg);
    if (msglen > E.screencols) msglen = E.screencols;
    if (msglen && time(NULL) - E.statusmsg_time < 5)   {
        abufAppend(buf, E.statusmsg, msglen);
    }
}

void refreshScreen(void)
{
    scroll();

    struct abuf ab = ABUF_INIT;

    abufAppend(&ab, "\x1b[?25l", 6);
    abufAppend(&ab, "\x1b[H", 3);

    drawRows(&ab);
    drawStatusBar(&ab);
    drawMessageBar(&ab);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
    abufAppend(&ab, buf, strlen(buf));

    abufAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.buf, ab.len);
    abufFree(&ab);
}

void setStatusMessage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

/*** input ***/

char* prompt(char *prompt, void (*callback)(const char*, const int))
{
    size_t bufcapacity = 128;
    char *buf = malloc(bufcapacity);

    size_t bufsize = 0;
    buf[0] = '\0';

    while (1)   {
        setStatusMessage(prompt, buf);
        refreshScreen();

        int c = readKey();
        if (c == DELETE_KEY || c == CTRL_KEY('h') || c == BACKSPACE)    {
            if (bufsize) buf[--bufsize] = '\0';
        } else if (c == '\x1b')    {
            setStatusMessage("");
            if (callback) callback(buf, c);
            free(buf);
            return NULL;
        } else if (c == '\r')  {
            if (bufsize)   {
                setStatusMessage("");
                if (callback) callback(buf, c);
                return buf;
            }
        } else if (!iscntrl(c) && c < 128)  {
            if (bufsize == bufcapacity - 1) {
                bufcapacity *= 2;
                buf = realloc(buf, bufcapacity);
            }
            buf[bufsize++] = c;
            buf[bufsize] = '\0'; 
        }

        if (callback) callback(buf, c);
    }
}

void moveCursor(const int key)
{
    erow *row = (E.cy >= E.rowsnum) ? NULL : &E.row[E.cy]; 

    switch (key)    {
        case ARROW_LEFT:
            if (E.cx != 0)  {
                E.cx--;
            } else if (E.cy > 0)    {
                E.cy--;
                E.cx = E.row[E.cy].size;
            }
            break;

        case ARROW_DOWN:
            if (E.cy < E.rowsnum)   {
                E.cy++;
            }
            break;

        case ARROW_RIGHT:
            if (row && E.cx < row->size)    {
                E.cx++;
            } else if (row && E.cx == row->size)    {
                E.cy++;
                E.cx = 0;
            }
            break;

        case ARROW_UP:
            if (E.cy != 0)  {
                E.cy--;
            }
            break;
    }

    row = (E.cy >= E.rowsnum) ? NULL : &E.row[E.cy];
    const int rowlen = row ? row->size : 0;
    if (E.cx > rowlen)  {
        E.cx = rowlen;
    } 
}

void processKeypress(void)
{
    static int quit_times = EDITOR_QUIT_TIMES;
    const int c = readKey();

    switch (c)  {
        case '\r':
            insertNewLine();
            break;

        case CTRL_KEY('q'):
            if (E.dirty && quit_times > 0)  {
                setStatusMessage("WARNING! File has unsaved changes.\nPress CTRL-Q %d more time to quit.", quit_times);
                quit_times--;
                return;
            }
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(EXIT_SUCCESS);
            break;

        case CTRL_KEY('s'):
            saveFile();
            break;

        case HOME_KEY:
            E.cx = 0;
            break;

        case END_KEY:
            if (E.cy < E.rowsnum)   {
                E.cx = E.row[E.cy].size;
            }
            break;

        case CTRL_KEY('f'):
            search();
            break;

        case BACKSPACE:
        case CTRL_KEY('h'):
        case DELETE_KEY:
            if (c == DELETE_KEY) moveCursor(ARROW_RIGHT);
            deleteChar();
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                if (c == PAGE_UP)   {
                    E.cy = E.rowoff;
                } else if (c == PAGE_DOWN)  {
                    E.cy = E.rowoff + E.screenrows - 1;
                    if (E.cy > E.rowsnum) E.cy = E.rowsnum;
                }
            }
            break;

        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            moveCursor(c);
            break;

        case CTRL_KEY('l'):
        case '\x1b':
            break;

        default:
            insertChar(c);
            break;
    }

    quit_times = EDITOR_QUIT_TIMES;
}

/*** init ***/

void initialize(void)   
{
    E.cx = 0;
    E.cy = 0;
    E.rx = 0;
    E.coloff = 0;
    E.rowoff = 0;
    E.rowsnum = 0;
    E.row = NULL;
    E.dirty = 0;
    E.filename = NULL;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;
    E.syntax = NULL;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1)  {
        die("getWindowSize");
    }

    E.screenrows -= 2;
}

int main(int argc, const char *argv[])  
{
    enableRawMode(); 
    initialize();

    if (argc >= 2)  {
        openFile(argv[1]);
    }

    setStatusMessage("HELP: CTRL-S = save | Ctrl-Q = quit | CTRL-F = search");

    while (1)   {
        refreshScreen();
        processKeypress();
    }

    return 0;
}
