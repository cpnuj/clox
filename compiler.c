#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "value.h"

#include "compiler.h"

static void emit_byte (Compiler *, uint8_t);
static void emit_bytes (Compiler *, uint8_t, uint8_t);
static void emit_constant (Compiler *, Value);
static void emit_return (Compiler *);

static uint8_t make_constant (Chunk *, Value);

void compiler_init (Compiler *, char *);
void compiler_set_chunk (Compiler *, Chunk *);
Token peek (Compiler *);
Token prev (Compiler *);
Token forward (Compiler *);
Token consume (Compiler *, TokenType);

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

typedef void (*nudFn) (Compiler *);
typedef void (*ledFn) (Compiler *);
nudFn token_nud (Token token);
ledFn token_led (Token token);
bindingPower token_bp (Token token);

void parse_expr (Compiler *compiler, bindingPower bp);
void parse_literal (Compiler *compiler);
void parse_negative (Compiler *compiler);
void parse_group (Compiler *compiler);
OpCode binary_opcode_from_token (Token token);
void parse_binary_op (Compiler *compiler);

static void emit_byte (Compiler *compiler, uint8_t byte)
{
  chunk_write (compiler->chunk, byte, compiler->prev.line);
}

static void emit_bytes (Compiler *compiler, uint8_t b1, uint8_t b2)
{
  emit_byte (compiler, b1);
  emit_byte (compiler, b2);
}

static void emit_constant (Compiler *compiler, Value value)
{
  // TODO: Do not make new constant to value array for bool and nil
  // to reduce memory usage.
  // We have initialized constant for bool and nil now, but need compiler
  // error handling.
  emit_bytes (compiler, OP_CONSTANT, make_constant (compiler->chunk, value));
}

static void emit_return (Compiler *compiler)
{
  emit_byte (compiler, OP_RETURN);
}

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

void compiler_init (Compiler *compiler, char *src)
{
  lex_init (&compiler->lexer, src, strlen (src));
  // initial forward
  forward (compiler);
}

void compiler_set_chunk (Compiler *compiler, Chunk *chunk)
{
  compiler->chunk = chunk;
}

Token peek (Compiler *compiler) { return compiler->curr; }

Token prev (Compiler *compiler) { return compiler->prev; }

Token forward (Compiler *compiler)
{
  compiler->prev = compiler->curr;
  compiler->curr = lex (&compiler->lexer);
  return compiler->prev;
}

Token consume (Compiler *compiler, TokenType type)
{
  Token token = peek (compiler);
  // FIXME: Replace assert with error handling
  assert (token_type (&token) == type);
  return forward (compiler);
}

void compile (char *src, Chunk *chunk)
{
  Compiler compiler;
  compiler_init (&compiler, src);
  compiler_set_chunk (&compiler, chunk);
  parse_expr (&compiler, BP_NONE);
  consume (&compiler, TK_EOF);

  emit_byte (&compiler, OP_RETURN);
}

// Pratt parsing algorithm

void parse_expr (Compiler *compiler, bindingPower bp)
{
  nudFn nud = token_nud (forward (compiler));
  if (nud)
  {
    nud (compiler);
  }
  while (bp < token_bp (peek (compiler)))
  {
    ledFn led = token_led (forward (compiler));
    assert (led);
    led (compiler);
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

void parse_literal (Compiler *compiler)
{
  Value v;

  Token token = prev (compiler);
  switch (token_type (&token))
  {
    // nil and bool values are already stored in chunk's constant array
    case TK_NIL:
      emit_bytes (compiler, OP_CONSTANT, constant_nil);
      return;
    case TK_FALSE:
      emit_bytes (compiler, OP_CONSTANT, constant_false);
      return;
    case TK_TRUE:
      emit_bytes (compiler, OP_CONSTANT, constant_true);
      return;

    // number and string literals are added to chunk's constant array at
    // runtime
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

  emit_constant (compiler, v);
}

void parse_negative (Compiler *compiler)
{
  // TODO: Is it right to use BP_UNARY ?
  parse_expr (compiler, BP_UNARY);
  emit_byte (compiler, OP_NEGATIVE);
}

void parse_group (Compiler *compiler)
{
  parse_expr (compiler, BP_NONE);
  consume (compiler, TK_RIGHT_PAREN);
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

void parse_binary_op (Compiler *compiler)
{
  Token token = prev (compiler);
  parse_expr (compiler, token_bp (token));
  emit_byte (compiler, binary_opcode_from_token (token));
}
