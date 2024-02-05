#include "hldb.h"

#include <stdlib.h>

#include "hlfiletypes.h"
#include "esyntax.h"
#include "hlhelpers.h"

struct esyntax HLDB[] = {
    {
      "c",
      C_FILETYPES,
      C_KEYWORDS,
      "//", "/*", "*/",
      HL_HIGHLIGHT_NUMBERS | HL_HIGHLIGHT_STRINGS
    },
};

int main()
{
    return EXIT_SUCCESS;
}
