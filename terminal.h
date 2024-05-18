#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdint.h>

void die(char const *s);

void disableRawMode(void);

void enableRawMode(void);

int32_t readKey(void);

int32_t getCursorPosition(int32_t *rows, int32_t *cols);

int16_t getWindowSize(int32_t *rows, int32_t *cols);

#endif /* TERMINAL_H */
