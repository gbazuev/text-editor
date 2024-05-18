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

int32_t convertCxToRx(erow *row, int32_t const cx);

int32_t convertRxToCx(erow *row, int32_t const rx);

void updateRow(erow *row);

void insertRow(int32_t const index, char const *s, size_t const len);

void freeRow(erow *row);

void deleteRow(int32_t const index);

void insertCharInRow(erow *row, int32_t index, int32_t const ch);

void appendStringInRow(erow *row, char const *str, size_t const len);

void deleteCharFromRow(erow *row, int32_t const index);

#endif /* EROW_H */
