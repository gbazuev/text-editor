/*** includes ***/

#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <ctype.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)
#define EDITOR_TAB_STOP 8
#define EDITOR_VERSION "0.0.1"

enum arrowsKeys {
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

/*** data ***/

typedef struct erow {
    int size;
    int rendersize;
    char *chars;
    char *render;
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
    char *filename; 
    char statusmsg[80];
    time_t statusmsg_time;
    struct termios original_termios;    //previous terminal state (for recover after exiting editor)
};

struct config E;

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
}

void appendRow(const char *s, const size_t len) 
{
    E.row = realloc(E.row, sizeof(erow) * (E.rowsnum + 1));
    const int at = E.rowsnum;

    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';

    E.row[at].rendersize = 0;
    E.row[at].render = NULL;
    updateRow(&E.row[at]);

    E.rowsnum++;
}

/*** file io ***/

void open(const char *filename)
{   
    free(E.filename);
    E.filename = strdup(filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line, &linecap, fp)) != -1)  {
        while ((linelen > 0 && (line[linelen - 1] == '\n')) || (line[linelen - 1] == '\r')) {
            linelen--;
        }
        appendRow(line, linelen);
    }

    free(line);
    fclose(fp);
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
            abufAppend(ab, &E.row[filerow].render[E.coloff], len);
        }

    abufAppend(ab, "\x1b[K", 3);
    abufAppend(ab, "\r\n", 2);
  }
}

void drawStatusBar(struct abuf *buf)    
{
    abufAppend(buf, "\x1b[7m", 4);
    char status[80], rstatus[80];

    int len = snprintf(status, sizeof(status), "%.20s - %d lines",
        E.filename ? E.filename : "[NO NAME]", E.rowsnum);
    int rlen = snprintf(rstatus, sizeof(rstatus), "%d/%d", E.cy + 1, E.rowsnum);

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
    const int c = readKey();

    switch (c)  {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(EXIT_SUCCESS);
            break;

        case HOME_KEY:
            E.cx = 0;
            break;

        case END_KEY:
            if (E.cy < E.rowsnum)   {
                E.cx = E.row[E.cy].size;
            }
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
    }
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
    E.filename = NULL;
    E.statusmsg[0] = '\0';
    E.statusmsg_time = 0;

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
        open(argv[1]);
    }

    setStatusMessage("HELP: Ctrl-Q = quit");

    while (1)   {
        refreshScreen();
        processKeypress();
    }

    return 0;
}
