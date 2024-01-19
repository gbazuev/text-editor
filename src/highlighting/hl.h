#ifndef HL_H
#define HL_H 

#include "src/row/erow.h"

int is_separator(int c);

void updateSyntax(erow *row);

int mapSyntaxToColor(const int highlight);

void selectSyntaxHighlight(void);

#endif /* HL_H */