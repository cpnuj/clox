#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "debug.h"
#include "lexer.h"

void interprete(char *src) {
    Lexer l;
    TokenType type;
    char lexem[128];

    LexInit(&l, src, strlen(src));
    for (;;) {
        Token token = Lex(&l);
        type = TokenGetType(&token);
        if (type == TK_ERR) {
            printf("ERROR: %s\n", LexError(&l));
            break;
        }
        if (type == TK_EOF) {
            break;
        }
        TokenGetLexem(&token, lexem);
        printf("Token: %d Lexem: %s\n", type, lexem);
    }
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

