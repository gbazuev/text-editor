#ifndef EROW_H
#define EROW_H

#include <stdint.h>
#include <stddef.h>

typedef struct erow {
    int32_t idx;
    int32_t size;
    int32_t rendersize;
    char *chars;
    char *render;
    char *highlight;
    int32_t hl_open_comment;
} erow;

int32_t convertCxToRx(erow *row, const int32_t cx);

int32_t convertRxToCx(erow *row, const int32_t rx);

void updateRow(erow *row);

void insertRow(const int32_t index, const char *s, const size_t len);

void freeRow(erow *row);

void deleteRow(const int32_t index);

void insertCharInRow(erow *row, int32_t index, const int32_t ch);

void appendStringInRow(erow *row, const char* str, const size_t len);

void deleteCharFromRow(erow *row, const int32_t index);

#endif /* EROW_H */