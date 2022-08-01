#include "memory.h"
#include <stdlib.h>

static int alloc_size = 0;

void *reallocate(void *ptr, int oldSize, int newSize)
{
  alloc_size += newSize - oldSize;
  if (newSize == 0) {
    free(ptr);
    return NULL;
  }
  return realloc(ptr, newSize);
}

int mem_alloc() { return alloc_size; }
