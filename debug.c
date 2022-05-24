#include "debug.h"
#include <stdio.h>

void debug_chunk (Chunk *chunk, char *name)
{
  printf ("== %s ==\n", name);
  for (int offset = 0; offset < chunk->len;)
  {
    offset = debug_instruction (chunk, offset);
  }
}

int debug_instruction (Chunk *chunk, int offset)
{
  printf ("%04d ", offset);
  if (offset > 0 && chunk->lines[offset] == chunk->lines[offset - 1])
  {
    printf ("   | ");
  }
  else
  {
    printf ("%4d ", chunk->lines[offset]);
  }

  uint8_t instruction = chunk->code[offset];
  switch (instruction)
  {
    case OP_RETURN:
      return simple_instruction ("OP_RETURN", offset);
    case OP_CONSTANT:
      return constant_instruction ("OP_CONSTANT", chunk, offset);
    case OP_ADD:
      return simple_instruction ("OP_ADD", offset);
    case OP_MINUS:
      return simple_instruction ("OP_MINUS", offset);
    case OP_MUL:
      return simple_instruction ("OP_MUL", offset);
    case OP_DIV:
      return simple_instruction ("OP_DIV", offset);
    case OP_NEGATIVE:
      return simple_instruction ("OP_NEGATIVE", offset);
    default:
      printf ("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}

int simple_instruction (char *name, int offset)
{
  printf ("%s\n", name);
  return offset + 1;
}

int constant_instruction (char *name, Chunk *chunk, int offset)
{
  int constant = chunk->code[offset + 1];
  printf ("%-16s %4d '", name, constant);
  value_print (chunk->constants.value[constant]);
  printf ("'\n");
  return offset + 2;
}
