#include "chunk.h"
#include "memory.h"

void chunk_init (Chunk *chunk)
{
  chunk->len = 0;
  chunk->cap = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  value_array_init (&chunk->constants);
}

void chunk_write (Chunk *chunk, uint8_t byte, int line)
{
  if (chunk->cap < chunk->len + 1)
  {
    int oldSize = chunk->cap;
    chunk->cap = grow_cap (chunk->cap);
    chunk->code = grow_array (uint8_t, chunk->code, oldSize, chunk->cap);
    chunk->lines = grow_array (int, chunk->lines, oldSize, chunk->cap);
  }
  chunk->code[chunk->len] = byte;
  chunk->lines[chunk->len] = line;
  chunk->len++;
}

void chunk_free (Chunk *chunk)
{
  value_array_free (&chunk->constants);
  free_array (uint8_t, chunk->code, chunk->cap);
  chunk_init (chunk);
}

int chunk_add_constant (Chunk *chunk, Value value)
{
  value_array_write (&chunk->constants, value);
  return chunk->constants.len - 1;
}
