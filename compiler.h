#ifndef clox_compiler_h
#define clox_compiler_h

#include "chunk.h"
#include "lexer.h"

typedef struct {
    Lexer lexer;
    Token curr;
    Token prev;
    Chunk *chunk;   // Compiling chunk
} Parser;

void Compile(char *src, Chunk *chunk);

#endif

