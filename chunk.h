#ifndef clox_chunk_h
#define clox_chunk_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "value.h"

typedef enum {
  OP_NONE,
  OP_RETURN,
  OP_CONSTANT,
  OP_NEGATIVE,
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
} op_code;

struct chunk {
  int len;
  int cap;
  int *lines;
  uint8_t *code;
  struct value_list constants;
};

#define constant_nil 0
#define constant_false 1
#define constant_true 2

void chunk_init(struct chunk *chunk);
void chunk_write(struct chunk *chunk, uint8_t byte, int line);
void chunk_free(struct chunk *chunk);

int chunk_add_constant(struct chunk *chunk, struct value value);

#endif
