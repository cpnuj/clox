#ifndef clox_debug_h
#define clox_debug_h

#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "value.h"

#define panic(s)                                                               \
  {                                                                            \
    printf("%s\n", s);                                                         \
    exit(1);                                                                   \
  }

void debug_chunk(Chunk *, ValueArray *, char *);
int debug_instruction(Chunk *, ValueArray *, int);

int simple_instruction(char *, int);
int constant_instruction(char *, Chunk *, ValueArray *, int);
int jmp_instruction(char *, Chunk *, int, int);

#endif
