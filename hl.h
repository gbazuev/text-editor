#ifndef HL_H
#define HL_H 

#include <stdint.h>

#include "erow.h"

int32_t is_separator(int32_t c);

void updateSyntax(erow *row);

int mapSyntaxToColor(int32_t const highlight);

void selectSyntaxHighlight(void);

#endif /* HL_H */
