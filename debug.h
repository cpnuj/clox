#ifndef clox_debug_h
#define clox_debug_h

#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"

#define panic(s)                                                               \
  {                                                                            \
    printf("%s\n", s);                                                         \
    exit(1);                                                                   \
  }

void debug_chunk(struct chunk *chunk, char *name);
int debug_instruction(struct chunk *chunk, int offset);

int simple_instruction(char *name, int offset);
int constant_instruction(char *name, struct chunk *chunk, int offset);

#endif
