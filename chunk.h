#ifndef clox_chunk_h
#define clox_chunk_h

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_RETURN,
} OpCode;

typedef struct {
    int len;
    int cap;
    int* lines;
    uint8_t* code;
    ValueArray constants;
} Chunk;

void initChunk(Chunk* chunk);
void writeChunk(Chunk* chunk, uint8_t byte, int line);
void freeChunk(Chunk* chunk);

int addConstant(Chunk* chunk, Value value);

#endif