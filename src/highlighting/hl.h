#ifndef HL_H
#define HL_H 

#include <stdint.h>

#include "row/erow.h"

int32_t is_separator(int32_t c);

void updateSyntax(erow *row);

int mapSyntaxToColor(const int32_t highlight);

void selectSyntaxHighlight(void);

#endif /* HL_H */