#ifndef HIGHLIGHTING_H
#define HIGHLIGHTING_H 

#include <stdint.h>
#include "system/erow.h"
#include "system/erow.h"
#include "system/config.h" 
#include "defines/keys.h"
#include "highlighting/hlhelpers.h"

int is_separator(int c);

void updateSyntax(erow *row);

int mapSyntaxToColor(const int highlight);

void selectSyntaxHighlight(void);

#endif /* HIGHLIGHTING_H */