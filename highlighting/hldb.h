#ifndef HLDB_H
#define HLDB_H

#include "hlfiletypes.h"
#include "syntax.h"
#include "hlhelpers.h"

extern struct editorSyntax HLDB;

#define HLDB_ENTRIES (sizeof(HLDB) / sizeof(HLDB[0]))

#endif /* HLDB_H */