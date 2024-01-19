#include "highlighting/hldb.h"

#include "src/highlighting/hlfiletypes.h"
#include "src/highlighting/syntax.h"
#include "src/highlighting/hlhelpers.h"

struct editorSyntax HIGHLIGHT_DB[] = {
     {
        "c",
        C_FILETYPES,
        C_KEYWORDS,
        "//", "/*", "*/",
        HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
     },
};