#include "hldb.h"

#include "hlfiletypes.h"
#include "esyntax.h"
#include "hlhelpers.h"

struct esyntax HLDB[1] = {
    {
      "c",
      C_FILETYPES,
      C_KEYWORDS,
      "//", "/*", "*/",
      HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
};
