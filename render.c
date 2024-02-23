#include "render.h"

#include <alloca.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "config.h"
#include "stringbuf.h"
#include "hlhelpers.h"
#include "hl.h"
#include "settings.h"
#include "algo.h" //for getNumberLength()

void scroll()   
{
    E.rx = 3;
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
        
        const int8_t maxline_numlen = getNumberLength(E.max_linenum);
        const int8_t actuline_numlen = getNumberLength(filerow + 1);

        if (filerow >= E.rowsnum) {
            if (E.rowsnum == 0 && y == E.screenrows / 3)   {
                char msg[80];
                int32_t msglen = snprintf(msg, sizeof(msg), "This is my pet-project text-editor! Version %s", EDITOR_VERSION);

                if (msglen > E.screencols) 
                    msglen = E.screencols;

                int32_t padding = (E.screencols - msglen) / 2;

                if (padding)    { 
                    stringbufAppend(buf, "\x1b[48;5;236m", 11);
                    stringbufAppend(buf, " ~ ", 3);
                    --padding;

                    while (padding--)  {
                        stringbufAppend(buf, "\x1b[", 2);
                        stringbufAppend(buf, HL_BACKGROUND, 9);
                        stringbufAppend(buf, " ", 1);
                    }

                    stringbufAppend(buf, msg, msglen);
                }
            } else {
                //TODO: add special flags like BACKGROUND_STRING in stringbufAppend proc.

                stringbufAppend(buf, "\x1b[48;5;236m", 11);
                stringbufAppend(buf, " ~ ", 3);
                stringbufAppend(buf, HL_RESET, 5);
                stringbufAppend(buf, "\x1b[", 2);
                stringbufAppend(buf, HL_BACKGROUND, 9);
            }
        } else {
            int32_t len = E.row[filerow].rendersize - E.coloff;
            if (len < 0) len = 0;
            if (len > E.screencols) len = E.screencols;
            char *c = &E.row[filerow].render[E.coloff]; //TODO: this should be put into a separate procedure
            char *highlight = &E.row[filerow].highlight[E.coloff];
            int32_t current_color = -1;
    
            stringbufAppend(buf, "\x1b[48;5;236m", 11);
            stringbufAppend(buf, " ", 1);
            char numbuf[16];
        
            char spacebuf[maxline_numlen - actuline_numlen + 1];
            memset(spacebuf, ' ', maxline_numlen - actuline_numlen);
        
            stringbufAppend(buf, spacebuf, maxline_numlen - actuline_numlen);
            int32_t numline_written = snprintf(numbuf, actuline_numlen + 1, "%d", filerow + 1);
            stringbufAppend(buf, numbuf, numline_written);

            stringbufAppend(buf, " ", 1);
            
            if (len == 0)   {
                stringbufAppend(buf, "\x1b[", 2);
                stringbufAppend(buf, HL_BACKGROUND, 9);
                goto ENDLINE;
            }

            for (int32_t j = 0; j < len; ++j)   {
                if (iscntrl(c[j]))  {
                    const char symbol = (c[j] <= 26) ? '@' + c[j] : '?';
                    stringbufAppend(buf, "\x1b[7m", 4);
                    stringbufAppend(buf, &symbol, 1);
                    if (current_color != -1) {
                        char cbuf[16];
                        int32_t clen = snprintf(cbuf, sizeof(cbuf), "\x1b[38;5;%d", current_color);
                        stringbufAppend(buf, "\x1b[", 2);
                        stringbufAppend(buf, HL_BACKGROUND, 9);
                        stringbufAppend(buf, cbuf, clen);
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

ENDLINE:
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
    stringbufAppend(buf, "\x1b[K\x1b[", 5);
    stringbufAppend(buf, HL_BACKGROUND, 9);
    int32_t msglen = strlen(E.statusmsg);
    if (msglen > E.screencols) msglen = E.screencols;
    if (msglen && time(NULL) - E.statusmsg_time < 5)   {
        stringbufAppend(buf, E.statusmsg, msglen);
    }

    while (msglen < E.screencols)   {
        stringbufAppend(buf, " ", 1);
        msglen++;
    }

    stringbufAppend(buf, "\x1b[m", 3);
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
