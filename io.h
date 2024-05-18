#ifndef IO_H
#define IO_H

#include <stdint.h>

char *convertRowsToSingleString(int32_t *buflen);

void openFile(char const *filename);

void saveFile(void);

#endif /* IO_H */
