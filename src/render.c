#include "render.h"

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>

#include "system/config.h"
#include "stringbuf/stringbuf.h"
#include "highlighting/hlhelpers.h"
#include "defines/settings.h"

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

void renderRows(struct stringbuf *ab)
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
                    abufAppend(ab, "\x1b[7m", 4); //TODO: highlighting!!!
                    abufAppend(ab, &symbol, 1);
                    abufAppend(ab, "\x1b[m", 3);
                    if (current_color != -1) {
                        char buf[16];
                        int clen = snprintf(buf, sizeof(buf), "\x1b[38;5;%d", current_color);
                        abufAppend(ab, buf, clen);
                        abufAppend(ab, HL_BACKGROUND, 9);
                    }
                } else if (highlight[j] == HL_NORMAL)  {
                    abufAppend(ab, HL_RESET, 5);
                    abufAppend(ab, "\x1b[", 2);
                    abufAppend(ab, HL_BACKGROUND, 9);
                    abufAppend(ab, &c[j], 1);
                } else {
                    int color = mapSyntaxToColor(highlight[j]);
                    char buf[16];
                    int clen = snprintf(buf, sizeof(buf), "\x1b[38;5;%d;", color); //COLOR length
                    abufAppend(ab, buf, clen);
                    abufAppend(ab, HL_BACKGROUND, 9);
                    abufAppend(ab, &c[j], 1);
                }
            }

            abufAppend(ab, HL_RESET, 5);
        }

    abufAppend(ab, "\x1b[K", 3);
    abufAppend(ab, "\r\n", 2);
  }
}

void renderStatusBar(struct stringbuf *buf)    
{
    abufAppend(buf, "\x1b[48;5;238m", 11);
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

void renderMessageBar(struct stringbuf *buf)
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

    struct stringbuf sbuf = STRINGBUF_INIT;

    abufAppend(&sbuf, "\x1b[?25l", 6);
    abufAppend(&sbuf, "\x1b[H", 3);

    drawRows(&sbuf);
    drawStatusBar(&sbuf);
    drawMessageBar(&sbuf);

    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
    abufAppend(&sbuf, buf, strlen(buf));

    abufAppend(&sbuf, "\x1b[?25h", 6);

    write(STDOUT_FILENO, sbuf.str, sbuf.len);
    abufFree(&sbuf);
}

void setStatusMessage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}