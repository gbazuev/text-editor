#include "renderbuf.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

void renderbufAppend(struct renderbuf *buf, char const *s, uint32_t const len)   
{
    char* new = realloc(buf->str, buf->len + len);

    if (new == NULL) return;
    memcpy(&new[buf->len], s, len);
    buf->str = new;
    buf->len += len;
}

void renderbufFree(struct renderbuf *buf) 
{
    free(buf->str);
}
