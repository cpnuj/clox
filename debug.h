#ifndef clox_debug_h
#define clox_debug_h

#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"

#define panic(s) { printf("%s\n", s); exit(1); }

void debug_chunk(Chunk* chunk, char* name);
int debug_instruction(Chunk* chunk, int offset);

int simple_instruction(char* name, int offset);
int constant_instruction(char* name, Chunk* chunk, int offset);

#endif
