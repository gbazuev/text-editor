#ifndef TERMINAL_H
#define TERMINAL_H

void die(const char *s);

void disableRawMode(void);

void enableRawMode(void);

int readKey(void);

int getCursorPosition(int *rows, int *cols);

short getWindowSize(int *rows, int *cols);

#endif /* TERMINAL_H */