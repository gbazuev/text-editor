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

/*** data ***/

struct config {
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

char readKey(void)
{
    int nread;
    char c;
    
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)    {
        if (nread == -1 && errno != EAGAIN) {
            die("read");
        }
    }
    
    return c;
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
        abufAppend(ab, "~", 1);

        if (y < E.screenrows - 1)    {
            abufAppend(ab, "\r\n", 2);
        }
   }
}

void refreshScreen(void)
{
    struct abuf ab = ABUF_INIT;

    abufAppend(&ab, "\x1b[2J", 4);
    abufAppend(&ab, "\x1b[H", 3);

    drawRows(&ab);

    abufAppend(&ab, "\x1b[H", 3);

    write(STDOUT_FILENO, ab.buf, ab.len);
    abufFree(&ab);
}

/*** input ***/

void processKeypress(void)
{
    char c = readKey();

    switch (c)  {
        case CTRL_KEY('q'):
            write(STDOUT_FILENO, "\x1b[2J", 4);
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(EXIT_SUCCESS);
            break;
    }
}

/*** init ***/

void initialize()   
{
    if (getWindowSize(&E.screenrows, &E.screencols) == -1)  {
        die("getWindowSize");
    }
}

int main()  
{
    enableRawMode(); 
    initialize();

    while (1)   {
        refreshScreen();
        processKeypress();
    }

    return 0;
}