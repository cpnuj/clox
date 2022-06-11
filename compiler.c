#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "value.h"

#include "compiler.h"

static void emit_byte(struct compiler *, uint8_t);
static void emit_bytes(struct compiler *, uint8_t, uint8_t);
static void emit_constant(struct compiler *, struct value);
static void emit_return(struct compiler *);

static uint8_t make_constant(struct chunk *, struct value);

void compiler_init(struct compiler *, char *);
void compiler_set_chunk(struct compiler *, struct chunk *);
struct token peek(struct compiler *);
struct token prev(struct compiler *);
struct token forward(struct compiler *);
struct token consume(struct compiler *, token_t);

typedef enum {
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
} binding_power;

typedef void (*nud_func)(struct compiler *);
typedef void (*led_func)(struct compiler *);
nud_func token_nud(struct token token);
led_func token_led(struct token token);
binding_power token_bp(struct token token);

void parse_expr(struct compiler *compiler, binding_power bp);
void parse_literal(struct compiler *compiler);
void parse_negative(struct compiler *compiler);
void parse_group(struct compiler *compiler);
op_code binary_opcode_from_token(struct token token);
void parse_binary_op(struct compiler *compiler);

static void emit_byte(struct compiler *compiler, uint8_t byte)
{
  chunk_write(compiler->chunk, byte, compiler->prev.line);
}

static void emit_bytes(struct compiler *compiler, uint8_t b1, uint8_t b2)
{
  emit_byte(compiler, b1);
  emit_byte(compiler, b2);
}

static void emit_constant(struct compiler *compiler, struct value value)
{
  // TODO: Do not make new constant to value array for bool and nil
  // to reduce memory usage.
  // We have initialized constant for bool and nil now, but need compiler
  // error handling.
  emit_bytes(compiler, OP_CONSTANT, make_constant(compiler->chunk, value));
}

static void emit_return(struct compiler *compiler)
{
  emit_byte(compiler, OP_RETURN);
}

static uint8_t make_constant(struct chunk *chunk, struct value value)
{
  int constant = chunk_add_constant(chunk, value);
  if (constant > UINT8_MAX) {
    // FIXME: add error
    return 0;
  }
  return (uint8_t)constant;
}

void compiler_init(struct compiler *compiler, char *src)
{
  lex_init(&compiler->lexer, src, strlen(src));
  // initial forward
  forward(compiler);
}

void compiler_set_chunk(struct compiler *compiler, struct chunk *chunk)
{
  compiler->chunk = chunk;
}

struct token peek(struct compiler *compiler) { return compiler->curr; }

struct token prev(struct compiler *compiler) { return compiler->prev; }

struct token forward(struct compiler *compiler)
{
  compiler->prev = compiler->curr;
  compiler->curr = lex(&compiler->lexer);
  return compiler->prev;
}

struct token consume(struct compiler *compiler, token_t type)
{
  struct token token = peek(compiler);
  // FIXME: Replace assert with error handling
  assert(token_type(&token) == type);
  return forward(compiler);
}

void compile(char *src, struct chunk *chunk)
{
  struct compiler compiler;
  compiler_init(&compiler, src);
  compiler_set_chunk(&compiler, chunk);
  parse_expr(&compiler, BP_NONE);
  consume(&compiler, TK_EOF);

  emit_byte(&compiler, OP_RETURN);
}

// Pratt parsing algorithm

void parse_expr(struct compiler *compiler, binding_power bp)
{
  nud_func nud = token_nud(forward(compiler));
  if (nud) {
    nud(compiler);
  }
  while (bp < token_bp(peek(compiler))) {
    led_func led = token_led(forward(compiler));
    assert(led);
    led(compiler);
  }
}

nud_func token_nud(struct token token)
{
  token_t t = token_type(&token);
  switch (t) {
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

led_func token_led(struct token token)
{
  token_t t = token_type(&token);
  switch (t) {
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

binding_power token_bp(struct token token)
{
  token_t t = token_type(&token);
  switch (t) {
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

void parse_literal(struct compiler *compiler)
{
  struct value v;

  struct token token = prev(compiler);
  switch (token_type(&token)) {
  // nil and bool values are already stored in chunk's constant array
  case TK_NIL:
    emit_bytes(compiler, OP_CONSTANT, constant_nil);
    return;
  case TK_FALSE:
    emit_bytes(compiler, OP_CONSTANT, constant_false);
    return;
  case TK_TRUE:
    emit_bytes(compiler, OP_CONSTANT, constant_true);
    return;

  // number and string literals are added to chunk's constant array at
  // runtime
  case TK_NUMBER: {
    double number = strtod(token_lexem_start(&token), NULL);
    v = value_make_number(number);
    break;
  }
  case TK_STRING: {
    v = value_make_string(token_lexem_start(&token) + 1,
                          token_lexem_len(&token) - 2);
    break;
  }
  }

  emit_constant(compiler, v);
}

void parse_negative(struct compiler *compiler)
{
  // TODO: Is it right to use BP_UNARY ?
  parse_expr(compiler, BP_UNARY);
  emit_byte(compiler, OP_NEGATIVE);
}

void parse_group(struct compiler *compiler)
{
  parse_expr(compiler, BP_NONE);
  consume(compiler, TK_RIGHT_PAREN);
}

op_code binary_opcode_from_token(struct token token)
{
  switch (token_type(&token)) {
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

void parse_binary_op(struct compiler *compiler)
{
  struct token token = prev(compiler);
  parse_expr(compiler, token_bp(token));
  emit_byte(compiler, binary_opcode_from_token(token));
}
