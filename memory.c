#include <stdlib.h>
#include "memory.h"

void* reallocate(void* ptr, int oldSize, int newSize) {
    if (newSize == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, newSize);
}