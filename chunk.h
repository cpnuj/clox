#ifndef clox_chunk_h
#define clox_chunk_h

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

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
  OP_PRINT,
  OP_POP,
  OP_CLOSE,
  OP_GLOBAL, // define global variable
  OP_LOCAL,  // define local variable
  OP_SET_GLOBAL,
  OP_GET_GLOBAL,
  OP_SET_LOCAL,
  OP_GET_LOCAL,
  OP_SET_UPVALUE,
  OP_GET_UPVALUE,
  OP_JMP,          // directly jump
  OP_JMP_BACK,     // directly jump backward
  OP_JMP_ON_FALSE, // condition jump
  OP_CLOSURE,
  OP_CALL,
  OP_CLASS,
  OP_GET_FIELD,
  OP_SET_FIELD,
  OP_METHOD,
  OP_INVOKE,
  OP_DERIVE,
  OP_GET_SUPER,
} op_code;

typedef struct {
  int len;
  int cap;
  int *lines;
  uint8_t *code;
} Chunk;

void chunk_init(Chunk *chunk);
void chunk_add(Chunk *chunk, uint8_t byte, int line);
void chunk_set(Chunk *chunk, int offset, uint8_t byte);
void chunk_free(Chunk *chunk);
int chunk_len(Chunk *chunk);

#endif
