#ifndef CONFIG_H
#define CONFIG_H

#include <time.h>
#include <termios.h>

#include "row/erow.h"

struct config {
    int cx, cy;                         //cursor coordinates (cx - row, cy - column)
    int rx;                             //render string horizontal coordinate
    int coloff;
    int rowoff;
    int screenrows;
    int screencols;
    int rowsnum;
    erow *row;                          //file rows
    int dirty;
    char *filename; 
    char statusmsg[80];
    time_t statusmsg_time;
    struct syntax *syntax;
    struct termios original_termios;    //previous terminal state (for recover after exiting editor)
};

extern struct config E;

void init(void);

#endif /* CONFIG_H */