#include "search.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "keys.h"
#include "hlhelpers.h"
#include "input.h"

void searchCallback(char const *query, int32_t const key)
{
    static int32_t last_match = -1;
    static int32_t direction = 1;

    static int32_t saved_highlight_line;
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
    int32_t current = last_match;

    for (int32_t i = 0; i < E.rowsnum; ++i) {
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
    int32_t saved_cx = E.cx, saved_cy = E.cy;
    int32_t saved_coloff = E.coloff, saved_rowoff = E.rowoff;

    char *query = prompt("Search: %s (Use ESC/Arrows/Enter)", searchCallback);
    if (query) free(query);
    else {
        E.cx = saved_cx;
        E.cy = saved_cy;
        E.coloff = saved_coloff;
        E.rowoff = saved_rowoff;
    }
}
