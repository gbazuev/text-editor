#include <stdlib.h>

#include "input.h"
#include "render.h"
#include "terminal.h"
#include "config.h"
#include "io.h"

int main(int argc, const char *argv[])
{
    enableRawMode();
    init();

    if (argc >= 2) {
        openFile(argv[1]);
    }

    setStatusMessage("HELP: Ctrl-S = save | Ctrl-Q = quit | Ctrl-F = find");

    while (1)   {
        refreshScreen();
        processKeypress();
    }
    
    return EXIT_SUCCESS;
}
