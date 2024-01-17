#ifndef STRINGBUF_H
#define STRINGBUF_H

struct stringbuf {
    char* str;
    unsigned int len;
};

#define STRINGBUF_INIT {NULL, 0}

void stringbufAppend(struct stringbuf *buf, const char *s, const uint32_t len);

void stringbufFree(struct stringbuf *buf);

#endif /* STRINGBUF_H */