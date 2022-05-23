#ifndef clox_debug_h
#define clox_debug_h

#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"

#define panic(s) { printf("%s\n", s); exit(1); }

void disassembleChunk(Chunk* chunk, char* name);
int disassembleInstruction(Chunk* chunk, int offset);

int simpleInstruction(char* name, int offset);
int constantInstruction(char* name, Chunk* chunk, int offset);

#endif
