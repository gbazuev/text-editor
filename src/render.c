#include "render.h"

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "system/config.h"
#include "stringbuf/stringbuf.h"
#include "highlighting/hlhelpers.h"
#include "highlighting/hl.h"
#include "defines/settings.h"
#include "render.h"

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

void renderRows(struct stringbuf *buf)
{
   int32_t y;
   for (y = 0; y < E.screenrows; ++y)  {
        const int32_t filerow = y + E.rowoff;

        if (filerow >= E.rowsnum) {
            if (E.rowsnum == 0 && y == E.screenrows / 3)   {
                char msg[80];
                int32_t msglen = snprintf(msg, sizeof(msg), "This is my pet-project text-editor! Version %s", EDITOR_VERSION);

                if (msglen > E.screencols) 
                    msglen = E.screencols;

                int32_t padding = (E.screencols - msglen) / 2;

                if (padding)    {
                    stringbufAppend(buf, "~", 1);
                    --padding;
                }

                while (padding--)  stringbufAppend(buf, " ", 1);

                stringbufAppend(buf, msg, msglen);
            } else {
                stringbufAppend(buf, "~", 1);
            }
        } else {
            int32_t len = E.row[filerow].rendersize - E.coloff;
            if (len < 0) len = 0;
            if (len > E.screencols) len = E.screencols;
            char *c = &E.row[filerow].render[E.coloff]; //TODO: this should be put into a separate procedure
            char *highlight = &E.row[filerow].highlight[E.coloff];
            int32_t current_color = -1;

            for (int32_t j = 0; j < len; ++j)   {
                if (iscntrl(c[j]))  {
                    const char symbol = (c[j] <= 26) ? '@' + c[j] : '?';
                    stringbufAppend(buf, "\x1b[7m", 4); //TODO: highlighting!!!
                    stringbufAppend(buf, &symbol, 1);
                    stringbufAppend(buf, "\x1b[m", 3);
                    if (current_color != -1) {
                        char cbuf[16];
                        int32_t clen = snprintf(cbuf, sizeof(cbuf), "\x1b[38;5;%d", current_color);
                        stringbufAppend(buf, cbuf, clen);
                        stringbufAppend(buf, HL_BACKGROUND, 9);
                    }
                } else if (highlight[j] == HL_NORMAL)  {
                    stringbufAppend(buf, HL_RESET, 5);
                    stringbufAppend(buf, "\x1b[", 2);
                    stringbufAppend(buf, HL_BACKGROUND, 9);
                    stringbufAppend(buf, &c[j], 1);
                } else {
                    int32_t color = mapSyntaxToColor(highlight[j]);
                    char cbuf[16];
                    int32_t clen = snprintf(cbuf, sizeof(cbuf), "\x1b[38;5;%d;", color); //COLOR length
                    stringbufAppend(buf, cbuf, clen);
                    stringbufAppend(buf, HL_BACKGROUND, 9);
                    stringbufAppend(buf, &c[j], 1);
                }
            }

            stringbufAppend(buf, HL_RESET, 5);
        }

    stringbufAppend(buf, "\x1b[K", 3);
    stringbufAppend(buf, "\r\n", 2);
  }
}

void renderStatusBar(struct stringbuf *buf)    
{
    stringbufAppend(buf, "\x1b[48;5;238m", 11);
    char status[80], rstatus[80];

    int32_t len = snprintf(status, sizeof(status), "%.20s - %d lines %s",
        E.filename ? E.filename : "[NO NAME]", E.rowsnum, E.dirty ? "(modified)" : "");

    int32_t rlen = snprintf(rstatus, sizeof(rstatus), "%s | %d/%d", E.syntax ? E.syntax->filetype : "no filetype", E.cy + 1, E.rowsnum);

    if (len > E.screencols) len = E.screencols;
    stringbufAppend(buf, status, len);

    while (len < E.screencols)  {
        if (E.screencols - len == rlen) {
            stringbufAppend(buf, rstatus, rlen);
            break;
        }
        
        stringbufAppend(buf, " ", 1);
        len++;
    }

    stringbufAppend(buf, "\x1b[m", 3);
    stringbufAppend(buf, "\r\n", 2);
}

void renderMessageBar(struct stringbuf *buf)
{
    stringbufAppend(buf, "\x1b[K", 3);
    int32_t msglen = strlen(E.statusmsg);
    if (msglen > E.screencols) msglen = E.screencols;
    if (msglen && time(NULL) - E.statusmsg_time < 5)   {
        stringbufAppend(buf, E.statusmsg, msglen);
    }
}

void refreshScreen(void)
{
    scroll();

    struct stringbuf sbuf = STRINGBUF_INIT;

    stringbufAppend(&sbuf, "\x1b[?25l", 6);
    stringbufAppend(&sbuf, "\x1b[H", 3);

    renderRows(&sbuf);
    renderStatusBar(&sbuf);
    renderMessageBar(&sbuf);

    char cbuf[32];
    snprintf(cbuf, sizeof(cbuf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.rx - E.coloff) + 1);
    stringbufAppend(&sbuf, cbuf, strlen(cbuf));

    stringbufAppend(&sbuf, "\x1b[?25h", 6);

    write(STDOUT_FILENO, sbuf.str, sbuf.len);
    stringbufFree(&sbuf);
}

void setStatusMessage(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}