#include "ProductionCode.h"
#include <stdlib.h>
#include <stdio.h>

static inline size_t random(const size_t n)
{
    return (size_t)rand() % n;
}

void shuffleSort(char* const arr, const size_t count)
{
    size_t i;

    // Iterate backwards, swapping last element of each iteration with a random element of lower index
    for(i = count; i-- > 0;)
    {
        size_t z = random(i) + 1;
        char tmp = arr[i];
        arr[i] = arr[z];
        arr[z] = tmp;

        fprintf(stderr, "arr[%zu] == arr[%zu]\n", i, z);
    }
}
