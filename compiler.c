#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "value.h"

#include "compiler.h"

static void emitByte (Parser *parser, uint8_t byte);
static void emitBytes (Parser *parser, uint8_t b1, uint8_t b2);
static void emitConstant (Parser *parser, Value value);
static void emitReturn (Parser *parser);

static uint8_t makeConstant (Chunk *chunk, Value value);

void parserInit (Parser *parser, char *src);
void parserSetChunk (Parser *p, Chunk *chunk);
Token parserPeek (Parser *parser);
Token parserPrev (Parser *parser);
Token parserForward (Parser *parser);
Token parserConsume (Parser *parser, TokenType type);

typedef enum
{
  BP_NONE,
  BP_ASSIGNMENT, // =
  BP_OR,         // or
  BP_AND,        // and
  BP_EQUALITY,   // == !=
  BP_COMPARISON, // < > <= >=
  BP_TERM,       // + -
  BP_FACTOR,     // * /
  BP_UNARY,      // ! -
  BP_CALL,       // . ()
  BP_PRIMARY
} bindingPower;

typedef void (*nudFn) (Parser *);
typedef void (*ledFn) (Parser *);
nudFn tokenNud (Token token);
ledFn tokenLed (Token token);
bindingPower tokenBindingPower (Token token);

void parseExpression (Parser *p, bindingPower bp);
void parseLiteral (Parser *parser);
void parseNegative (Parser *p);
void parseGroup (Parser *p);
OpCode binaryOpCodeFromToken (Token token);
void parseBinaryOp (Parser *p);

static void emitByte (Parser *parser, uint8_t byte)
{
  writeChunk (parser->chunk, byte, parser->prev.line);
}

static void emitBytes (Parser *parser, uint8_t b1, uint8_t b2)
{
  emitByte (parser, b1);
  emitByte (parser, b2);
}

static void emitConstant (Parser *parser, Value value)
{
  // TODO: Do not make new constant to value array for bool and nil
  // to reduce memory usage.
  emitBytes (parser, OP_CONSTANT, makeConstant (parser->chunk, value));
}

static void emitReturn (Parser *parser) { emitByte (parser, OP_RETURN); }

static uint8_t makeConstant (Chunk *chunk, Value value)
{
  int constant = addConstant (chunk, value);
  if (constant > UINT8_MAX)
  {
    // FIXME: add error
    return 0;
  }
  return (uint8_t)constant;
}

void parserInit (Parser *parser, char *src)
{
  LexInit (&parser->lexer, src, strlen (src));
  // initial forward
  parserForward (parser);
}

void parserSetChunk (Parser *p, Chunk *chunk) { p->chunk = chunk; }

Token parserPeek (Parser *parser) { return parser->curr; }

Token parserPrev (Parser *parser) { return parser->prev; }

Token parserForward (Parser *p)
{
  p->prev = p->curr;
  p->curr = Lex (&p->lexer);
  return p->prev;
}

Token parserConsume (Parser *p, TokenType type)
{
  Token token = parserPeek (p);
  // FIXME: Replace assert with error handling
  assert (TokenGetType (&token) == type);
  return parserForward (p);
}

void Compile (char *src, Chunk *chunk)
{
  Parser p;
  parserInit (&p, src);
  parserSetChunk (&p, chunk);
  parseExpression (&p, BP_NONE);
  parserConsume (&p, TK_EOF);

  emitByte (&p, OP_RETURN);
}

// Pratt parsing algorithm

void parseExpression (Parser *p, bindingPower bp)
{
  nudFn nud = tokenNud (parserForward (p));
  if (nud)
  {
    nud (p);
  }
  while (bp < tokenBindingPower (parserPeek (p)))
  {
    ledFn led = tokenLed (parserForward (p));
    assert (led);
    led (p);
  }
}

nudFn tokenNud (Token token)
{
  TokenType t = TokenGetType (&token);
  switch (t)
  {
    case TK_MINUS:
      return parseNegative;
    case TK_LEFT_PAREN:
      return parseGroup;
    case TK_NIL:
    case TK_TRUE:
    case TK_FALSE:
    case TK_NUMBER:
      return parseLiteral;
  }
  return 0;
}

ledFn tokenLed (Token token)
{
  TokenType t = TokenGetType (&token);
  switch (t)
  {
    case TK_MINUS:
    case TK_PLUS:
    case TK_STAR:
    case TK_SLASH:
    case TK_BANG_EQUAL:
    case TK_EQUAL_EQUAL:
    case TK_GREATER:
    case TK_GREATER_EQUAL:
    case TK_LESS:
    case TK_LESS_EQUAL:
    case TK_AND:
    case TK_OR:
      return parseBinaryOp;
  }
  return 0;
}

bindingPower tokenBindingPower (Token token)
{
  TokenType t = TokenGetType (&token);
  switch (t)
  {
    case TK_MINUS:
    case TK_PLUS:
      return BP_TERM;
    case TK_STAR:
    case TK_SLASH:
      return BP_FACTOR;
    case TK_BANG_EQUAL:
    case TK_EQUAL_EQUAL:
      return BP_EQUALITY;
    case TK_GREATER:
    case TK_GREATER_EQUAL:
    case TK_LESS:
    case TK_LESS_EQUAL:
      return BP_COMPARISON;
    case TK_AND:
      return BP_AND;
    case TK_OR:
      return BP_OR;
  }
  return BP_NONE;
}

void parseLiteral (Parser *p)
{
  Value v;
  double number;

  Token token = parserPrev (p);
  switch (TokenGetType (&token))
  {
    case TK_NUMBER:
      number = strtod (TokenGetLexemStart (&token), NULL);
      v = NewNumValue (number);
      break;
    case TK_TRUE:
      v = NewBoolValue (true);
      break;
    case TK_FALSE:
      v = NewBoolValue (false);
      break;
    case TK_NIL:
      // TODO
      return;
  }
  emitConstant (p, v);
}

void parseNegative (Parser *p)
{
  // TODO: Is it right to use BP_UNARY ?
  parseExpression (p, BP_UNARY);
  emitByte (p, OP_NEGATIVE);
}

void parseGroup (Parser *p)
{
  parseExpression (p, BP_NONE);
  parserConsume (p, TK_RIGHT_PAREN);
}

OpCode binaryOpCodeFromToken (Token token)
{
  switch (TokenGetType (&token))
  {
    case TK_PLUS:
      return OP_ADD;
    case TK_MINUS:
      return OP_MINUS;
    case TK_STAR:
      return OP_MUL;
    case TK_SLASH:
      return OP_DIV;
    case TK_BANG_EQUAL:
      return OP_BANG_EQUAL;
    case TK_EQUAL_EQUAL:
      return OP_EQUAL_EQUAL;
    case TK_GREATER:
      return OP_GREATER;
    case TK_GREATER_EQUAL:
      return OP_GREATER_EQUAL;
    case TK_LESS:
      return OP_LESS;
    case TK_LESS_EQUAL:
      return OP_LESS_EQUAL;
    case TK_AND:
      return OP_AND;
    case TK_OR:
      return OP_OR;
    default:
      return OP_NONE;
  }
}

void parseBinaryOp (Parser *p)
{
  Token token = parserPrev (p);
  parseExpression (p, tokenBindingPower (token));
  emitByte (p, binaryOpCodeFromToken (token));
}
