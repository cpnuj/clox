#include <stdio.h>

#include "debug.h"
#include "object.h"

void debug_chunk(Chunk *chunk, ValueArray *constants, char *name)
{
  printf("== %s ==\n", name);
  for (int offset = 0; offset < chunk->len;) {
    offset = debug_instruction(chunk, constants, offset);
  }
}

int debug_instruction(Chunk *chunk, ValueArray *constants, int offset)
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
  case OP_PRINT:
    return simple_instruction("OP_PRINT", offset);
  case OP_POP:
    return simple_instruction("OP_POP", offset);
  case OP_CLOSE:
    return simple_instruction("OP_CLOSE", offset);

  case OP_CONSTANT:
    return constant_instruction("OP_CONSTANT", chunk, constants, offset);
  case OP_GLOBAL:
    return constant_instruction("OP_GLOBAL", chunk, constants, offset);
  case OP_LOCAL:
    return simple_instruction("OP_LOCAL", offset);
  case OP_SET_GLOBAL:
    return constant_instruction("OP_SET_GLOBAL", chunk, constants, offset);
  case OP_GET_GLOBAL:
    return constant_instruction("OP_GET_GLOBAL", chunk, constants, offset);
  case OP_SET_LOCAL:
    return constant_instruction("OP_SET_LOCAL", chunk, NULL, offset);
  case OP_GET_LOCAL:
    return constant_instruction("OP_GET_LOCAL", chunk, NULL, offset);
  case OP_SET_UPVALUE:
    return constant_instruction("OP_SET_UPVALUE", chunk, NULL, offset);
  case OP_GET_UPVALUE:
    return constant_instruction("OP_GET_UPVALUE", chunk, NULL, offset);

  case OP_JMP:
    return jmp_instruction("OP_JMP", chunk, 1, offset);
  case OP_JMP_BACK:
    return jmp_instruction("OP_JMP_BACK", chunk, -1, offset);
  case OP_JMP_ON_FALSE:
    return jmp_instruction("OP_JMP_ON_FALSE", chunk, 1, offset);

  case OP_CLOSURE:
    offset = constant_instruction("OP_CLOSURE", chunk, constants, offset);
    int constant = chunk->code[offset - 1];
    ObjectFunction *fun = value_as_fun(constants->value[constant]);
    for (int i = 0; i < fun->upvalue_size; i++) {
      int idx = chunk->code[offset++];
      int from_local = chunk->code[offset++];
      printf("%04d    |                       %s %d\n", offset - 2,
             from_local ? "local" : "upvalue", idx);
    }
    return offset;

  case OP_CALL:
    return constant_instruction("OP_CALL", chunk, NULL, offset);

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

int constant_instruction(char *name, Chunk *chunk, ValueArray *constants,
                         int offset)
{
  int constant = chunk->code[offset + 1];
  printf("%-16s %4d '", name, constant);
  if (constants != NULL) {
    value_print(constants->value[constant]);
  }
  printf("'\n");
  return offset + 2;
}

int jmp_instruction(char *name, Chunk *chunk, int sign, int offset)
{
  int h8 = chunk->code[offset + 1]; // high 8 bit
  int l8 = chunk->code[offset + 2]; // low  8 bit
  int dist = (h8 << 8) | l8;
  printf("%-16s %4d\n", name, sign * dist);
  return offset + 3;
}
