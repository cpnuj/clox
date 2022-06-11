#include "chunk.h"
#include "memory.h"

void chunk_init(struct chunk *chunk)
{
  chunk->len = 0;
  chunk->cap = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  value_array_init(&chunk->constants);

  // Make sure the order of initial constants sync with constant_* macros.
  chunk_add_constant(chunk, value_make_nil());
  chunk_add_constant(chunk, value_make_bool(false));
  chunk_add_constant(chunk, value_make_bool(true));
}

void chunk_write(struct chunk *chunk, uint8_t byte, int line)
{
  if (chunk->cap < chunk->len + 1) {
    int oldSize = chunk->cap;
    chunk->cap = grow_cap(chunk->cap);
    chunk->code = grow_array(uint8_t, chunk->code, oldSize, chunk->cap);
    chunk->lines = grow_array(int, chunk->lines, oldSize, chunk->cap);
  }
  chunk->code[chunk->len] = byte;
  chunk->lines[chunk->len] = line;
  chunk->len++;
}

void chunk_free(struct chunk *chunk)
{
  value_array_free(&chunk->constants);
  free_array(uint8_t, chunk->code, chunk->cap);
  chunk_init(chunk);
}

int chunk_add_constant(struct chunk *chunk, struct value value)
{
  value_array_write(&chunk->constants, value);
  return chunk->constants.len - 1;
}
