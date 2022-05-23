#include "debug.h"
#include <stdio.h>

void disassembleChunk (Chunk *chunk, char *name)
{
  printf ("== %s ==\n", name);
  for (int offset = 0; offset < chunk->len;)
  {
    offset = disassembleInstruction (chunk, offset);
  }
}

int disassembleInstruction (Chunk *chunk, int offset)
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
      return simpleInstruction ("OP_RETURN", offset);
    case OP_CONSTANT:
      return constantInstruction ("OP_CONSTANT", chunk, offset);
    case OP_ADD:
      return simpleInstruction ("OP_ADD", offset);
    case OP_MINUS:
      return simpleInstruction ("OP_MINUS", offset);
    case OP_MUL:
      return simpleInstruction ("OP_MUL", offset);
    case OP_DIV:
      return simpleInstruction ("OP_DIV", offset);
    case OP_NEGATIVE:
      return simpleInstruction ("OP_NEGATIVE", offset);
    default:
      printf ("Unknown opcode %d\n", instruction);
      return offset + 1;
  }
}

int simpleInstruction (char *name, int offset)
{
  printf ("%s\n", name);
  return offset + 1;
}

int constantInstruction (char *name, Chunk *chunk, int offset)
{
  int constant = chunk->code[offset + 1];
  printf ("%-16s %4d '", name, constant);
  printValue (chunk->constants.value[constant]);
  printf ("'\n");
  return offset + 2;
}
