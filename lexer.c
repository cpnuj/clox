#include <stdlib.h>
#include <stdio.h>

#include "lexer.h"
#include "keyword.h"

void lexerSkipWhitespace(Lexer *l);
int lexerEnd(Lexer *l);
char lexerPeek(Lexer *l);
char lexerPeekNext(Lexer *l);
char lexerForward(Lexer *l);
int lexerMatch(Lexer *l, char c);
void lexerSetError(Lexer *l, char *errmsg);

int isDigit(char c);
int isLetter(char c);
int isKeyword(char *s, int len);

Token makeToken(Lexer *l, TokenType type);

TokenType lexNumber(Lexer *l);
TokenType lexIdent(Lexer *l);
TokenType lexString(Lexer *l);

void lexerSkipWhitespace(Lexer *l) {
    for (;;) {
        char c = lexerPeek(l);
        switch (c) {
            case ' ':
            case '\n':
            case '\r':
            case '\t':
                lexerForward(l);
                break;
            // comment
            case '/':
                if (lexerPeekNext(l) == '/') {
                    while (!lexerEnd(l) && lexerForward(l) != '\n'){}
                    return;
                }
            default:
                return;
        }
    }
}

int lexerEnd(Lexer *l) {
    return l->end >= l->len;
}

char lexerPeek(Lexer *l) {
    return lexerEnd(l) ? 0 : l->src[l->end];
}

char lexerPeekNext(Lexer *l) {
    return (l->end + 1 >= l->len) ? 0 : l->src[l->end + 1];
}

char lexerForward(Lexer *l) {
    if (lexerEnd(l)) {
        return 0;
    }
    char ret = l->src[l->end];
    l->end++;
    return ret;
}

int lexerMatch(Lexer *l, char c) {
    if (lexerPeek(l) == c) {
        lexerForward(l);
        return 1;
    }
    return 0;
}

void lexerSetError(Lexer *l, char *errmsg) {
    l->err = 1;
    l->errmsg = errmsg;
}

int isDigit(char c) {
    return c >= '0' && c <= '9';
}

int isLetter(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_');
}

int isKeyword(char *s, int len) {
    return CloxKeyword(s, len);
}

Token makeToken(Lexer *l, TokenType type) {
    Token tk = {
        .type = type,
        .at   = l->src + l->start,
        .len  = l->end - l->start,
    };
    return tk;
}

TokenType lexNumber(Lexer *l) {
    int hasDot = 0;
    for (;;) {
        char peek = lexerPeek(l);
        if (isDigit(peek)) {
            lexerForward(l);
        } else if (peek == '.') {
            if (hasDot) {
                lexerSetError(l, "expect digit");
                return TK_ERR;
            }
            hasDot = 1;
            lexerForward(l);
        } else {
            return TK_NUMBER;
        }
    }
}

TokenType lexIdent(Lexer *l) {
    char c = lexerPeek(l);
    while ( isDigit(c) || isLetter(c) ) {
        lexerForward(l);
        c = lexerPeek(l);
    }
    int tk = isKeyword(l->src + l->start, l->end - l->start);
    if (tk > 0) {
        return tk;
    }
    return TK_IDENT;
}

TokenType lexString(Lexer *l) {
    while (lexerForward(l) != '"') {
        if (lexerEnd(l)) {
            lexerSetError(l, "unclosed \" for string literal");
            return TK_ERR;
        }
    }
    return TK_STRING;
}

Lexer* LexNew(char *src, int len) {
    Lexer *l = (Lexer*)malloc(sizeof(Lexer));
    l->start = 0;
    l->end = 0;
    l->len = len;
    l->src = src;
    l->err = 0;
    return l;
}

void LexInit(Lexer *l, char *src, int len) {
    l->start = 0;
    l->end = 0;
    l->len = len;
    l->src = src;
    l->err = 0;
}

Token Lex(Lexer *l) {
    if (l->err) return makeToken(l, TK_ERR);

    lexerSkipWhitespace(l);
    if (lexerEnd(l)) return makeToken(l, TK_EOF);

    l->start = l->end;

    char c = lexerForward(l);
    // notation
    switch(c) {
        // Single-character tokens.
        case '(': return makeToken(l, TK_LEFT_PAREN);
        case ')': return makeToken(l, TK_RIGHT_PAREN);
        case '{': return makeToken(l, TK_LEFT_BRACE);
        case '}': return makeToken(l, TK_RIGHT_BRACE);
        case ',': return makeToken(l, TK_COMMA);
        case '.': return makeToken(l, TK_DOT);
        case '-': return makeToken(l, TK_MINUS);
        case '+': return makeToken(l, TK_PLUS);
        case ';': return makeToken(l, TK_SEMICOLON);
        case '/': return makeToken(l, TK_SLASH);
        case '*': return makeToken(l, TK_STAR);

        // Single-character tokens.
        case '!': return makeToken(l, lexerMatch(l, '=') ?
                                   TK_BANG_EQUAL : TK_BANG);
        case '=': return makeToken(l, lexerMatch(l, '=') ?
                                   TK_EQUAL_EQUAL : TK_EQUAL);
        case '>': return makeToken(l, lexerMatch(l, '=') ?
                                   TK_GREATER_EQUAL : TK_GREATER);
        case '<': return makeToken(l, lexerMatch(l, '=') ?
                                   TK_LESS_EQUAL : TK_LESS);
    }

    // string
    if (c == '"') return makeToken(l, lexString(l));
    // number
    if (isDigit(c)) return makeToken(l, lexNumber(l));
    // identifier
    if (isLetter(c)) return makeToken(l, lexIdent(l));
    // error
    lexerSetError(l, "lex error");
    return makeToken(l, TK_ERR);
}

char* LexError(Lexer *l) {
    if (l->err) {
        return l->errmsg;
    }
    return 0;
}

TokenType TokenGetType(Token *token) {
    return token->type;
}

char* TokenGetLexem(Token *token, char *dest) {
    sprintf(dest, "%.*s", token->len, token->at);
    return dest;
}

