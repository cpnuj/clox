#ifndef clox_compiler_h
#define clox_compiler_h

#include "chunk.h"
#include "lexer.h"
#include "map.h"
#include "object.h"
#include "value.h"

// local_var represents a local variable with its name
// and depth in the scope chain.
typedef struct {
  int depth;
  bool is_captured;
  Value name;
} Local;

typedef struct {
  int idx;
  bool from_local;
  Value name;
} UpValue;

typedef struct scope {
  int sp;
  int cur_depth;
  Local locals[UINT8_MAX + 1];
  int upvalue_size;
  UpValue upvalues[UINT8_MAX + 1];
  struct scope *enclosing;
} Scope;

typedef struct {
  Lexer lexer;
  Token curr;
  Token prev;
  ValueArray *constants;
  Map interned_strings;
  Map mconstants; // map from value to idx in the constant list
  Scope *cur_scope;
  Chunk *cur_chunk; // Compiling chunk

  int error;
  int panic;
  char errmsg[128];
} Compiler;

int compile(char *, ObjectFunction *, ValueArray *);

#endif
