#include "chunk.h"
#include "memory.h"

void initChunk(Chunk* chunk) {
    chunk->len = 0;
    chunk->cap = 0;
    chunk->code = NULL;
    chunk->lines = NULL;
    initValueArray(&chunk->constants);
}

void writeChunk(Chunk* chunk, uint8_t byte, int line) {
    if (chunk->cap < chunk->len+1) {
        int oldSize = chunk->cap;
        chunk->cap = GROW_CAP(chunk->cap);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, oldSize, chunk->cap);
        chunk->lines = GROW_ARRAY(int, chunk->lines, oldSize, chunk->cap);
    }
    chunk->code[chunk->len] = byte;
    chunk->lines[chunk->len] = line;
    chunk->len++;
}

void freeChunk(Chunk* chunk) {
    freeValueArray(&chunk->constants);
    FREE_ARRAY(uint8_t, chunk->code, chunk->cap);
    initChunk(chunk);
}

int addConstant(Chunk* chunk, Value value) {
    writeValueArray(&chunk->constants, value);
    return chunk->constants.len - 1;
}

