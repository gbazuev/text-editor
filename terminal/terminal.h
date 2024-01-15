#ifndef TERMINAL_H
#define TERMINAL_H

#include <termios.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "defines/keys.h"
#include "system/config.h"

void die(const char *s);

void disableRawMode(void);

void enableRawMode(void);

int readKey(void);

int getCursorPosition(int *rows, int *cols);

#endif /* TERMINAL_H */