/*** includes ***/

#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

/*** defines ***/

#define CTRL_KEY(k) ((k) & 0x1f)
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

struct config {
    int cx, cy;
    int screenrows;
    int screencols;
    struct termios original_termios;
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

void drawRows(struct abuf *ab)
{
   int y;
   for (y = 0; y < E.screenrows; ++y)  {
    if (y == E.screenrows / 3)   {
        char msg[80u];
        int msglen = snprintf(msg, sizeof(msg), "This is my pet-project text-editor! Version %s", EDITOR_VERSION);

        if (msglen > E.screencols) msglen = E.screencols;
        int padding = (E.screencols - msglen) / 2;

        if (padding)    {
            abufAppend(ab, "~", 1);
            --padding;
        }
        while (padding--) abufAppend(ab, " ", 1);
        abufAppend(ab, msg, msglen);
    } else {
        abufAppend(ab, "~", 1);
    }

        abufAppend(ab, "\x1b[K", 3);
        if (y < E.screenrows - 1)    {
            abufAppend(ab, "\r\n", 2);
        }
   }
}

void refreshScreen(void)
{
    struct abuf ab = ABUF_INIT;

    abufAppend(&ab, "\x1b[?25l", 6);
    abufAppend(&ab, "\x1b[H", 3);

    drawRows(&ab);

    char buf[32u];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", E.cy + 1, E.cx + 1);
    abufAppend(&ab, buf, strlen(buf));

    abufAppend(&ab, "\x1b[?25h", 6);

    write(STDOUT_FILENO, ab.buf, ab.len);
    abufFree(&ab);
}

/*** input ***/

void moveCursor(const int key)
{
    switch (key)    {
        case ARROW_LEFT:
            if (E.cx != 0)  {
                --E.cx;
            }
            break;

        case ARROW_DOWN:
            if (E.cy != E.screenrows - 1)   {
                ++E.cy;
            }
            break;

        case ARROW_RIGHT:
            if (E.cx != E.screencols - 1)   {
                ++E.cx;
            }
            break;

        case ARROW_UP:
            if (E.cy != 0)  {
                --E.cy;
            }
            break;
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
            E.cx = E.screencols - 1;
            break;

        case PAGE_UP:
        case PAGE_DOWN:
            {
                int times = E.screenrows;
                while (times--) {
                    moveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
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

    if (getWindowSize(&E.screenrows, &E.screencols) == -1)  {
        die("getWindowSize");
    }
}

int main(void)  
{
    enableRawMode(); 
    initialize();

    while (1)   {
        refreshScreen();
        processKeypress();
    }

    return 0;
}