#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "value.h"

#include "compiler.h"

static void emit_byte (Parser *parser, uint8_t byte);
static void emit_bytes (Parser *parser, uint8_t b1, uint8_t b2);
static void emit_constant (Parser *parser, Value value);
static void emit_return (Parser *parser);

static uint8_t make_constant (Chunk *chunk, Value value);

void parser_init (Parser *parser, char *src);
void parserSetChunk (Parser *p, Chunk *chunk);
Token parser_peek (Parser *parser);
Token parser_prev (Parser *parser);
Token parser_forward (Parser *parser);
Token parser_consume (Parser *parser, TokenType type);

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
nudFn token_nud (Token token);
ledFn token_led (Token token);
bindingPower token_bp (Token token);

void parse_expr (Parser *p, bindingPower bp);
void parse_literal (Parser *parser);
void parse_negative (Parser *p);
void parse_group (Parser *p);
OpCode binary_opcode_from_token (Token token);
void parse_binary_op (Parser *p);

static void emit_byte (Parser *parser, uint8_t byte)
{
  chunk_write (parser->chunk, byte, parser->prev.line);
}

static void emit_bytes (Parser *parser, uint8_t b1, uint8_t b2)
{
  emit_byte (parser, b1);
  emit_byte (parser, b2);
}

static void emit_constant (Parser *parser, Value value)
{
  // TODO: Do not make new constant to value array for bool and nil
  // to reduce memory usage.
  // We have initialized constant for bool and nil now, but need parser
  // error handling.
  emit_bytes (parser, OP_CONSTANT, make_constant (parser->chunk, value));
}

static void emit_return (Parser *parser) { emit_byte (parser, OP_RETURN); }

static uint8_t make_constant (Chunk *chunk, Value value)
{
  int constant = chunk_add_constant (chunk, value);
  if (constant > UINT8_MAX)
  {
    // FIXME: add error
    return 0;
  }
  return (uint8_t)constant;
}

void parser_init (Parser *parser, char *src)
{
  lex_init (&parser->lexer, src, strlen (src));
  // initial forward
  parser_forward (parser);
}

void parserSetChunk (Parser *p, Chunk *chunk) { p->chunk = chunk; }

Token parser_peek (Parser *parser) { return parser->curr; }

Token parser_prev (Parser *parser) { return parser->prev; }

Token parser_forward (Parser *p)
{
  p->prev = p->curr;
  p->curr = lex (&p->lexer);
  return p->prev;
}

Token parser_consume (Parser *p, TokenType type)
{
  Token token = parser_peek (p);
  // FIXME: Replace assert with error handling
  assert (token_type (&token) == type);
  return parser_forward (p);
}

void compile (char *src, Chunk *chunk)
{
  Parser p;
  parser_init (&p, src);
  parserSetChunk (&p, chunk);
  parse_expr (&p, BP_NONE);
  parser_consume (&p, TK_EOF);

  emit_byte (&p, OP_RETURN);
}

// Pratt parsing algorithm

void parse_expr (Parser *p, bindingPower bp)
{
  nudFn nud = token_nud (parser_forward (p));
  if (nud)
  {
    nud (p);
  }
  while (bp < token_bp (parser_peek (p)))
  {
    ledFn led = token_led (parser_forward (p));
    assert (led);
    led (p);
  }
}

nudFn token_nud (Token token)
{
  TokenType t = token_type (&token);
  switch (t)
  {
    case TK_MINUS:
      return parse_negative;
    case TK_LEFT_PAREN:
      return parse_group;
    case TK_NIL:
    case TK_TRUE:
    case TK_FALSE:
    case TK_NUMBER:
    case TK_STRING:
      return parse_literal;
  }
  return 0;
}

ledFn token_led (Token token)
{
  TokenType t = token_type (&token);
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
      return parse_binary_op;
  }
  return 0;
}

bindingPower token_bp (Token token)
{
  TokenType t = token_type (&token);
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

void parse_literal (Parser *p)
{
  Value v;

  Token token = parser_prev (p);
  switch (token_type (&token))
  {
    // nil and bool values are already stored in chunk's constant array
    case TK_NIL:
      emit_bytes (p, OP_CONSTANT, constant_nil);
      return;
    case TK_FALSE:
      emit_bytes (p, OP_CONSTANT, constant_false);
      return;
    case TK_TRUE:
      emit_bytes (p, OP_CONSTANT, constant_true);
      return;

    // number and string literals are added to chunk's constant array at runtime
    case TK_NUMBER:
    {
      double number = strtod (token_lexem_start (&token), NULL);
      v = value_make_number (number);
      break;
    }
    case TK_STRING:
    {
      v = value_make_string (token_lexem_start (&token) + 1,
                             token_lexem_len (&token) - 2);
      break;
    }
  }

  emit_constant (p, v);
}

void parse_negative (Parser *p)
{
  // TODO: Is it right to use BP_UNARY ?
  parse_expr (p, BP_UNARY);
  emit_byte (p, OP_NEGATIVE);
}

void parse_group (Parser *p)
{
  parse_expr (p, BP_NONE);
  parser_consume (p, TK_RIGHT_PAREN);
}

OpCode binary_opcode_from_token (Token token)
{
  switch (token_type (&token))
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

void parse_binary_op (Parser *p)
{
  Token token = parser_prev (p);
  parse_expr (p, token_bp (token));
  emit_byte (p, binary_opcode_from_token (token));
}
