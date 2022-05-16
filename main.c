#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "lexer.h"

void interprete(char *src) {
    Chunk chunk;
    initChunk(&chunk);
    Compile(src, &chunk);
    disassembleChunk(&chunk, "hello kitty");
    freeChunk(&chunk);
}

static void repl() {
    char line[1024];
    Lexer l;
    for (;;) {
        printf("> ");
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        interprete(line);
    }
}

int main(int argc, char** argv) {
    repl(); 
    return 0;
}

