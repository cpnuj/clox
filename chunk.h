#ifndef clox_chunk_h
#define clox_chunk_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "value.h"

typedef enum
{
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
} OpCode;

typedef struct
{
  int len;
  int cap;
  int *lines;
  uint8_t *code;
  ValueArray constants;
} Chunk;

void chunk_init (Chunk *chunk);
void chunk_write (Chunk *chunk, uint8_t byte, int line);
void chunk_free (Chunk *chunk);

int chunk_add_constant (Chunk *chunk, Value value);

#endif
