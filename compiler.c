#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "chunk.h"
#include "value.h"

#include "compiler.h"

static void emitByte(Parser *parser, uint8_t byte);
static void emitBytes(Parser *parser, uint8_t b1, uint8_t b2);
static void emitConstant(Parser *parser, Value value);
static void emitReturn(Parser *parser);

static uint8_t makeConstant(Chunk *chunk, Value value);

void parserInit(Parser *parser, char *src);
void parserSetChunk(Parser *p, Chunk *chunk);
Token parserPeek(Parser *parser);
Token parserPrev(Parser *parser);
Token parserForward(Parser *parser);

typedef enum {
    BP_NONE,
    BP_ASSIGNMENT,  // =
    BP_OR,          // or
    BP_AND,         // and
    BP_EQUALITY,    // == !=
    BP_COMPARISON,  // < > <= >=
    BP_TERM,        // + -
    BP_FACTOR,      // * /
    BP_UNARY,       // ! -
    BP_CALL,        // . ()
    BP_PRIMARY
} bindingPower;

typedef void (*nudFn)(Parser*);
typedef void (*ledFn)(Parser*);
nudFn tokenNud(Token token);
ledFn tokenLed(Token token);
bindingPower tokenBindingPower(Token token);

void parseExpression(Parser *p, bindingPower bp);
void parseNumber(Parser *parser);
void parseAdd(Parser *parser);

static void emitByte(Parser *parser, uint8_t byte) {
    writeChunk(parser->chunk, byte, parser->prev.line);
}

static void emitBytes(Parser *parser, uint8_t b1, uint8_t b2) {
    emitByte(parser, b1);
    emitByte(parser, b2);
}

static void emitConstant(Parser *parser, Value value) {
    emitBytes(parser, OP_CONSTANT, makeConstant(parser->chunk, value));
}

static void emitReturn(Parser *parser) {
    emitByte(parser, OP_RETURN);
}

static uint8_t makeConstant(Chunk *chunk, Value value) {
    int constant = addConstant(chunk, value);
    if (constant > UINT8_MAX) {
        // FIXME: add error
        return 0;
    }
    return (uint8_t)constant;
}

void parserInit(Parser *parser, char *src) {
    LexInit(&parser->lexer, src, strlen(src));
    // initial forward
    parserForward(parser);
}

void parserSetChunk(Parser *p, Chunk *chunk) {
    p->chunk = chunk;
}

Token parserPeek(Parser *parser) {
    return parser->curr;
}

Token parserPrev(Parser *parser) {
    return parser->prev;
}

Token parserForward(Parser *p) {
    p->prev = p->curr;
    p->curr = Lex(&p->lexer);
    return p->prev;
}

void Compile(char *src, Chunk *chunk) {
    Parser p;
    parserInit(&p, src);
    parserSetChunk(&p, chunk);
    parseExpression(&p, BP_NONE);
}

// Pratt parsing algorithm

void parseExpression(Parser *p, bindingPower bp) {
    nudFn nud = tokenNud(parserForward(p));
    if (nud) {
        nud(p);
    }
    while (bp < tokenBindingPower(parserPeek(p))) {
        ledFn led = tokenLed(parserForward(p));
        assert(led);
        led(p);
    }
}

nudFn tokenNud(Token token) {
    TokenType t = TokenGetType(&token);
    switch (t) {
        case TK_NUMBER:
            return parseNumber;
    }
    return 0;
}

ledFn tokenLed(Token token) {
    TokenType t = TokenGetType(&token);
    switch (t) {
        case TK_PLUS:
            return parseAdd;
    }
    return 0;
}

bindingPower tokenBindingPower(Token token) {
    TokenType t = TokenGetType(&token);
    switch (t) {
        case TK_PLUS:
            return BP_TERM;
    }
    return BP_NONE;
}

void parseNumber(Parser *p) {
    Token token = parserPrev(p);
    double value = strtod(TokenGetLexemStart(&token), NULL);
    emitConstant(p, value);
}

void parseAdd(Parser *p) {
    parseExpression(p, BP_TERM);
    emitByte(p, OP_ADD);
}

