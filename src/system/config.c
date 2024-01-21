#include "config.h"

#include <time.h>
#include <termios.h>

#include "row/erow.h"
#include "terminal/terminal.h"
#include "highlighting/esyntax.h"
#include "input.h"

struct config E;

void init(void)
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

int main(int argc, const char** const argv)
{
    init();
    return 0;
}