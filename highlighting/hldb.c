#include "hldb.h"

struct editorSyntax HIGHLIGHT_DB[] = {
     {
        "c",
        C_FILETYPES,
        C_KEYWORDS,
        "//", "/*", "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
     },
};