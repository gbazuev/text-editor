#ifndef HLDB_H
#define HLDB_H

#include "highlighting/esyntax.h"

extern struct esyntax HLDB[];

#define HLDB_SIZE (sizeof(HLDB) / sizeof(HLDB[0]))

#endif /* HLDB_H */