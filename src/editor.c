//TODO: add all headers that are needed

#include "terminal/terminal.h"
#include "system/config.h"

int main(int argc, const char *argv[])  
{
    enableRawMode(); 
    initialize();

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