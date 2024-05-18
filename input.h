#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>

char *prompt(char *prompt, void (*callback)(char const *, int32_t const));

void moveCursor(int32_t const key);

void processKeypress(void);

#endif /* INPUT_H */
