#pragma once

#include <termios.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "Defines/cuarrows.h"
#include "Defines/config.h"

void die(const char *s);

void disableRawMode(void);

void enableRawMode(void);

int readKey(void);

int getCursorPosition(int *rows, int *cols);