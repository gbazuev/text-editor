#include "erow.h"

#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "hl.h"
#include "settings.h"
#include "config.h"

int32_t convertCxToRx(erow* row, const int32_t cx)
{
    int32_t rx = 0;
    for (int32_t j = 0; j < cx; ++j)    {
        if (row->chars[j] == '\t')  {
            rx += (EDITOR_TAB_STOP - 1) - (rx % EDITOR_TAB_STOP);
        }
        rx++;
    }

    return rx;
}

int32_t convertRxToCx(erow *row, const int32_t rx)
{
    int32_t current_rx = 0, cx;
    for (cx = 0; cx < row->size; ++cx)  {
        if (row->chars[cx] == '\t')
            current_rx += (EDITOR_TAB_STOP - 1) - (current_rx % EDITOR_TAB_STOP);
        current_rx++;

        if (current_rx > rx) return cx;
    }

    return cx;
}

void updateRow(erow *row)
{
    int32_t tabs = 0, j;
    for (j = 0; j < row->size; ++j) {
        if (row->chars[j] == '\t') ++tabs;
    }

    free(row->render);
    row->render = malloc(row->size + tabs * (EDITOR_TAB_STOP - 1) + 1);

    int32_t idx = 0;
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

    updateSyntax(row);
}

void insertRow(const int32_t index, const char *s, const size_t len) 
{
    if (index < 0 || index > E.rowsnum) return;

    E.row = realloc(E.row, sizeof(erow) * (E.rowsnum + 1));
    memmove(&E.row[index + 1], &E.row[index], sizeof(erow) * (E.rowsnum - index));
    
    for (int32_t j = index + 1; j <= E.rowsnum; ++j)   E.row[j].idx++;

    E.row[index].idx = index;

    E.row[index].size = len;
    E.row[index].chars = malloc(len + 1);
    memcpy(E.row[index].chars, s, len);
    E.row[index].chars[len] = '\0';

    E.row[index].rendersize = 0;
    E.row[index].render = NULL;
    E.row[index].highlight = NULL;
    E.row[index].hl_open_comment = 0;
    updateRow(&E.row[index]);

    E.rowsnum++;
    E.dirty++;
}

void freeRow(erow *row)
{
    free(row->render);
    free(row->chars);
    free(row->highlight);
}

void deleteRow(const int32_t index)
{
    if (index < 0 || index >= E.rowsnum)  return;
    freeRow(&E.row[index]);
    memmove(&E.row[index], &E.row[index + 1], sizeof(erow) * (E.rowsnum - index - 1));
    
    for (int32_t j = index; j < E.rowsnum - 1; ++j) E.row[j].idx--;

    E.rowsnum--;
    E.dirty++;
}

void insertCharInRow(erow *row, int32_t index, const int32_t ch)
{
    if (index < 0 || index > row->size) index = row->size;
    row->chars = realloc(row->chars, row->size + 2);
    memmove(&row->chars[index + 1], &row->chars[index], row->size - index + 1);
    row->size++;
    row->chars[index] = ch;
    updateRow(row);
    E.dirty++;
}

void appendStringInRow(erow *row, const char *str, const size_t len)
{
    row->chars = realloc(row->chars, row->size + len + 1);
    memcpy(&row->chars[row->size], str, len);
    row->size += len;
    row->chars[row->size] = '\0';
    updateRow(row);
    E.dirty++;
}

void deleteCharFromRow(erow *row, const int32_t index)
{
    if (index < 0 || index >= row->size) return;
    memmove(&row->chars[index], &row->chars[index + 1], row->size - index);
    row->size--;
    updateRow(row);
    E.dirty++;
}
