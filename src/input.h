#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

char *prompt(char *prompt, void (*callback)(const char*, const int32_t));

void moveCursor(const int32_t key);

void processKeypress(void);

#endif /* INPUT_H */