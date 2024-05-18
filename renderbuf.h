#ifndef STRINGBUF_H
#define STRINGBUF_H

#include <stdint.h>

struct renderbuf {
    char* str;
    uint32_t len;
};

#define RENDERBUF_INIT {NULL, 0}

void renderbufAppend(struct renderbuf *buf, char const *s, uint32_t const len);

void renderbufFree(struct renderbuf *buf);

#endif /* STRINGBUF_H */
