#ifndef clox_compiler_h
#define clox_compiler_h

#include "chunk.h"
#include "lexer.h"

struct compiler {
  struct lexer lexer;
  struct token curr;
  struct token prev;
  struct chunk *chunk; // Compiling chunk
};

void compile(char *src, struct chunk *chunk);

#endif
