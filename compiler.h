#ifndef clox_compiler_h
#define clox_compiler_h

#include "chunk.h"
#include "lexer.h"
#include "value.h"

// local_var represents a local variable with its name
// and depth in the scope chain.
struct local_var {
  int depth;
  struct value name;
};

struct scope {
  int sp;
  int cur_depth;
  struct local_var locals[UINT8_MAX + 1];
};

struct compiler {
  struct lexer lexer;
  struct token curr;
  struct token prev;
  struct chunk *chunk; // Compiling chunk
  struct scope scope;
  struct map mconstants; // map from value to idx in the chunk's constant list

  int error;
  int panic;
  char errmsg[128];
};

int compile(char *src, struct chunk *chunk);

#endif
