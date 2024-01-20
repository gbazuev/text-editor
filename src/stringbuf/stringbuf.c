#include "stringbuf/stringbuf.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void stringbufAppend(struct stringbuf *buf, const char *s, const uint32_t len)   
{
    char* new = realloc(buf->str, buf->len + len);

    if (new == NULL) return;
    memcpy(&new[buf->len], s, len);
    buf->str = new;
    buf->len += len;
}

void abufFree(struct stringbuf *buf) 
{
    free(buf->str);
}