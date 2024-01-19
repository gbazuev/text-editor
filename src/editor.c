#include "src/terminal/terminal.h"
#include "src/system/config.h"
#include "src/input.h"
#include "src/render.h"
#include "src/io.h"

int main(int argc, const char *argv[])  
{
    enableRawMode(); 
    init();

    if (argc >= 2)  {
        openFile(argv[1]);
    }

    setStatusMessage("HELP: CTRL-S = save | Ctrl-Q = quit | CTRL-F = search");

    while (1)   {
        refreshScreen();
        processKeypress();
    }

    return 0;
}