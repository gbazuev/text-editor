#include "input.h"

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>

#include "render.h"
#include "keys.h"
#include "erow.h"
#include "terminal.h"
#include "config.h"
#include "settings.h"
#include "io.h"
#include "editing.h"
#include "search.h"

char *prompt(char *prompt, void (*callback)(char const *, int32_t const))
{
    size_t bufcapacity = 128;
    char *buf = malloc(bufcapacity);

    size_t bufsize = 0;
    buf[0] = '\0';

    while (1)   {
        setStatusMessage(prompt, buf);
        refreshScreen();

        int32_t c = readKey();
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

void moveCursor(int32_t const key)
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
    int32_t const rowlen = row ? row->size : 0;
    if (E.cx > rowlen)  {
        E.cx = rowlen;
    } 
}

void processKeypress(void)
{
    static int32_t quit_times = EDITOR_QUIT_TIMES;
    int32_t const c = readKey();

    switch (c)  {
        case '\r':
            insertNewLine();
            break;

        case CTRL_KEY('q'):
            if (E.dirty && quit_times > 0)  {
                setStatusMessage("WARNING! File has unsaved changes. Press CTRL-Q %d more time to quit.", quit_times);
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
