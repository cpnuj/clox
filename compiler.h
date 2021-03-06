#ifndef clox_compiler_h
#define clox_compiler_h

#include "chunk.h"
#include "lexer.h"
#include "map.h"
#include "value.h"

// local_var represents a local variable with its name
// and depth in the scope chain.
struct local {
  int depth;
  struct value name;
};

struct scope {
  int bp;
  int sp;
  int cur_depth;
  struct local locals[UINT8_MAX + 1];
};

struct compiler {
  struct lexer lexer;
  struct token curr;
  struct token prev;
  struct chunk *chunk; // Compiling chunk
  struct value_list *constants;
  struct scope scope;
  struct map mconstants; // map from value to idx in the constant list

  int error;
  int panic;
  char errmsg[128];
};

int compile(char *, struct obj_fun *, struct value_list *);

#endif
