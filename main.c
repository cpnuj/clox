#include <stdio.h>
#include <stdlib.h>

#include "chunk.h"
#include "debug.h"

int main(int argc, char** argv) {
    Chunk* chunk = (Chunk*)malloc(sizeof(Chunk));
    initChunk(chunk);

    writeChunk(chunk, OP_CONSTANT, 123);
    writeChunk(chunk, addConstant(chunk, 8.8), 123);
    writeChunk(chunk, OP_RETURN, 123);

    disassembleChunk(chunk, "test chunk");

    freeChunk(chunk);

    return 0;
}
