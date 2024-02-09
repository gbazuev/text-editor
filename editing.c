#include "editing.h"

#include "config.h"

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
  if (E.cx == 3)  {
    insertRow(E.cy, "", 0);
  } else {
    erow *row = &E.row[E.cy];
    insertRow(E.cy + 1, &row->chars[E.cx - 3], row->size + 3 - E.cx);
    row = &E.row[E.cy];
    row->size = E.cx - 3;
    row->chars[row->size] = '\0';
    updateRow(row);
  }

  E.cy++;
  E.cx = 3;
}

void deleteChar(void)
{   
  if (E.cy == E.rowsnum || (E.cx == 3 && E.cy == 0)) return;

  erow *row = &E.row[E.cy];
  if (E.cx > 3)   {
    deleteCharFromRow(row, E.cx - 1);
    E.cx--;
  } else {
    E.cx = E.row[E.cy - 1].size + 3;
    appendStringInRow(&E.row[E.cy - 1], row->chars, row->size);
    deleteRow(E.cy);
    E.cy--;
  }
}
