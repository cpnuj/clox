#ifndef clox_chunk_h
#define clox_chunk_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "map.h"
#include "value.h"

typedef enum {
  OP_NONE,
  OP_RETURN,
  OP_CONSTANT,
  OP_NEGATIVE,
  OP_NOT,
  OP_MINUS,
  OP_ADD,
  OP_MUL,
  OP_DIV,
  OP_BANG,
  OP_BANG_EQUAL,
  OP_EQUAL,
  OP_EQUAL_EQUAL,
  OP_GREATER,
  OP_GREATER_EQUAL,
  OP_LESS,
  OP_LESS_EQUAL,
  OP_AND,
  OP_OR,
  OP_PRINT,
  OP_POP,
  OP_GLOBAL, // define global variable
  OP_LOCAL,  // define local variable
  OP_SET_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_LOCAL,
} op_code;

struct chunk {
  int len;
  int cap;
  int *lines;

  uint8_t *code;
  struct value_list constants;
  struct map globals;
};

void chunk_init(struct chunk *chunk);
void chunk_write(struct chunk *chunk, uint8_t byte, int line);
void chunk_free(struct chunk *chunk);

int chunk_add_constant(struct chunk *chunk, struct value value);

#endif
