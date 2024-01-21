#include "highlighting/hldb.h"

#include "highlighting/hlfiletypes.h"
#include "highlighting/esyntax.h"
#include "highlighting/hlhelpers.h"

struct esyntax HLDB[] = {
     {
        "c",
        C_FILETYPES,
        C_KEYWORDS,
        "//", "/*", "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
     },
};