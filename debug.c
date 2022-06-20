#include "debug.h"
#include <stdio.h>

void debug_chunk(struct chunk *chunk, char *name)
{
  printf("== %s ==\n", name);
  for (int offset = 0; offset < chunk->len;) {
    offset = debug_instruction(chunk, offset);
  }
}

int debug_instruction(struct chunk *chunk, int offset)
{
  printf("%04d ", offset);
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1]) {
    printf("   | ");
  } else {
    printf("%4d ", chunk->lines[offset]);
  }

  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_RETURN:
    return simple_instruction("OP_RETURN", offset);
  case OP_ADD:
    return simple_instruction("OP_ADD", offset);
  case OP_MINUS:
    return simple_instruction("OP_MINUS", offset);
  case OP_MUL:
    return simple_instruction("OP_MUL", offset);
  case OP_DIV:
    return simple_instruction("OP_DIV", offset);
  case OP_NEGATIVE:
    return simple_instruction("OP_NEGATIVE", offset);
  case OP_NOT:
    return simple_instruction("OP_NOT", offset);
  case OP_BANG:
    return simple_instruction("OP_BANG", offset);
  case OP_BANG_EQUAL:
    return simple_instruction("OP_BANG_EQUAL", offset);
  case OP_EQUAL:
    return simple_instruction("OP_EQUAL", offset);
  case OP_EQUAL_EQUAL:
    return simple_instruction("OP_EQUAL_EQUAL", offset);
  case OP_GREATER:
    return simple_instruction("OP_GREATER", offset);
  case OP_GREATER_EQUAL:
    return simple_instruction("OP_GREATER_EQUAL", offset);
  case OP_LESS:
    return simple_instruction("OP_LESS", offset);
  case OP_LESS_EQUAL:
    return simple_instruction("OP_LESS_EQUAL", offset);
  case OP_AND:
    return simple_instruction("OP_AND", offset);
  case OP_OR:
    return simple_instruction("OP_OR", offset);
  case OP_PRINT:
    return simple_instruction("OP_PRINT", offset);
  case OP_POP:
    return simple_instruction("OP_POP", offset);

  case OP_CONSTANT:
    return constant_instruction("OP_CONSTANT", chunk, offset);
  case OP_GLOBAL:
    return constant_instruction("OP_GLOBAL", chunk, offset);
  case OP_LOCAL:
    return simple_instruction("OP_LOCAL", offset);
  case OP_SET_GLOBAL:
    return constant_instruction("OP_SET_GLOBAL", chunk, offset);
  case OP_GET_GLOBAL:
    return constant_instruction("OP_GET_GLOBAL", chunk, offset);
  case OP_SET_LOCAL:
    return simple_instruction("OP_SET_LOCAL", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}

int simple_instruction(char *name, int offset)
{
  printf("%s\n", name);
  return offset + 1;
}

int constant_instruction(char *name, struct chunk *chunk, int offset)
{
  int constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  value_print(chunk->constants.value[constant]);
  printf("'\n");
  return offset + 2;
}
