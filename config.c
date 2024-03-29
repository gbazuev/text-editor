#include "config.h"

#include <stdlib.h>
#include <time.h>
#include <termios.h>

#include "terminal.h"

struct config E;

void init(void)
{
    E.cx = 0; //because of line numbers rendering (space + number + space)
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
    E.max_linenum = 0;

    if (getWindowSize(&E.screenrows, &E.screencols) == -1)  {
        die("getWindowSize");
    }

    E.screenrows -= 2;
}
