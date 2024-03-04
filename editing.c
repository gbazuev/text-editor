#include "editing.h"

#include "config.h"
#include "erow.h"

void insertChar(const int8_t character)
{
  if (E.cy == E.rowsnum)   {
    insertRow(E.rowsnum, "", 0);
  }

  insertCharInRow(&E.row[E.cy], E.cx, character);
  E.cx++;
}

void insertNewLine(void) // "Enter" keypress
{
  if (E.cx == 0)  {
    insertRow(E.cy, "", 0);
  } else {
    erow *row = &E.row[E.cy];
    insertRow(E.cy + 1, &row->chars[E.cx], row->size - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx;
    row->chars[row->size] = '\0';
    updateRow(row);
  }

  E.cy++;
  E.cx = 0;
}

void deleteChar(void)
{   
  if (E.cy == E.rowsnum || (E.cx == 0 && E.cy == 0)) return;

  erow *row = &E.row[E.cy];
  if (E.cx > 0)   {
    deleteCharFromRow(row, E.cx - 1);
    E.cx--;
  } else {
    E.cx = E.row[E.cy - 1].size;
    appendStringInRow(&E.row[E.cy - 1], row->chars, row->size);
    deleteRow(E.cy);
    E.cy--;
  }
}
