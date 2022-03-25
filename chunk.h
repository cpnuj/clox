#ifndef clox_chunk_h
#define clox_chunk_h

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint8_t* code;
} Chunk;

#endif