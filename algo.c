#include "algo.h"

#include <stdint.h>

int8_t getNumberLength(int32_t number)
{
    int8_t numlen = 0;
    while (number)  {
        number /= 10;
        numlen++;
    }

    return numlen;
}
