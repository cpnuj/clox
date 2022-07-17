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

void debug_chunk(struct chunk *, struct value_list *, char *);
int debug_instruction(struct chunk *, struct value_list *, int);

int simple_instruction(char *, int);
int constant_instruction(char *, struct chunk *, struct value_list *, int);
int jmp_instruction(char *, struct chunk *, int, int);

#endif
