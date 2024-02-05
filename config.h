#ifndef CONFIG_H
#define CONFIG_H

#include <time.h>
#include <termios.h>
#include <stdint.h>

#include "erow.h"
#include "esyntax.h"

struct config {
    int32_t cx, cy;                         //cursor coordinates (cx - row, cy - column)
    int32_t rx;                             //render string horizontal coordinate
    int32_t coloff;
    int32_t rowoff;
    int32_t screenrows;
    int32_t screencols;
    int32_t rowsnum;
    erow *row;                          //file rows
    int32_t dirty;
    char *filename; 
    char statusmsg[80];
    time_t statusmsg_time;
    struct esyntax *syntax;
    struct termios original_termios;    //previous terminal state (for recover after exiting editor)
};

extern struct config E;

void init(void);

#endif /* CONFIG_H */
