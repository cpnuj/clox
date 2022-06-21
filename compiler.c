#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "value.h"

void scope_init(struct scope *);
void scope_in(struct scope *);
void scope_out(struct scope *);
void scope_add(struct scope *, struct value);
int scope_find(struct scope *, struct value);
void scope_debug(struct scope *);
bool is_global(struct scope *);

void compiler_init(struct compiler *, char *);
void compiler_set_chunk(struct compiler *, struct chunk *);

static void emit_byte(struct compiler *, uint8_t);
static void emit_bytes(struct compiler *, uint8_t, uint8_t);
static void emit_constant(struct compiler *, struct value);
static void emit_return(struct compiler *);

static uint8_t make_constant(struct compiler *, struct value);

struct token peek(struct compiler *);
struct token prev(struct compiler *);
struct token forward(struct compiler *);
struct token consume(struct compiler *, token_t);
bool check(struct compiler *, token_t);
bool match(struct compiler *, token_t);

typedef uint8_t binding_power;

#define BP_NONE 0
#define BP_ASSIGNMENT 10 // =
#define BP_OR 20         // or
#define BP_AND 30        // and
#define BP_EQUALITY 40   // == !=
#define BP_COMPARISON 50 // < > <= >=
#define BP_TERM 60       // + -
#define BP_FACTOR 70     // * /
#define BP_UNARY 80      // ! -
#define BP_CALL 90       // . ()
#define BP_PRIMARY 100

// struct detail stores the parsing info of detail expression,
// used by led_func.
// In our one-pass compiler, in general we parse a series of tokens
// and emit codes immediately. But resolving variables is an exception
// since we don't know we should set or get the value of the variable
// until we see the following tokens. So we store the information
// in this detail structure, and operators should eval these details
// in their own way.
// Currently, only literal and variable would be evaled effectively.
// Nud functions of literal and variable would not emit any code
// until its detail is evaled by the led of operators.
struct detail {
  token_t id;
  int arity;
  struct value first;
};

static struct detail empty_detail(token_t);
static struct detail unary_detail(token_t, struct value);

typedef struct detail (*nud_func)(struct compiler *);
typedef struct detail (*led_func)(struct compiler *, struct detail);
nud_func token_nud(struct token token);
led_func token_led(struct token token);
binding_power token_bp(struct token token);

struct detail expression(struct compiler *compiler, binding_power bp);
struct detail literal(struct compiler *compiler);
struct detail variable(struct compiler *compiler);
struct detail negative(struct compiler *compiler);
struct detail not(struct compiler * compiler);
struct detail group(struct compiler *compiler);
struct detail assignment(struct compiler *c, struct detail left);
struct detail infix(struct compiler *compiler, struct detail);
op_code infix_opcode(struct token token);

void parse_decl(struct compiler *compiler);
void parse_var_decl(struct compiler *compiler);
void parse_stmt(struct compiler *compiler);
void parse_expr_stmt(struct compiler *compiler);
void parse_print_stmt(struct compiler *compiler);
void parse_block(struct compiler *compiler);

void defvar(struct compiler *compiler, struct value name);
void setvar(struct compiler *compiler, struct value name);
void getvar(struct compiler *compiler, struct value name);
static void eval(struct compiler *, struct detail);

void scope_init(struct scope *scope)
{
  scope->sp = -1;
  scope->cur_depth = 0;
}

void scope_in(struct scope *scope) { scope->cur_depth++; }

void scope_out(struct scope *scope)
{
  // pop all locals belonging to current depth
  while (scope->sp >= 0) {
    if (scope->locals[scope->sp].depth != scope->cur_depth) {
      break;
    }
    scope->sp--;
  }
  scope->cur_depth--;
}

void scope_add(struct scope *scope, struct value name)
{
  scope->sp++;
  scope->locals[scope->sp].depth = scope->cur_depth;
  scope->locals[scope->sp].name = name;
}

int scope_find(struct scope *scope, struct value name)
{
  for (int at = scope->sp; at >= 0; at--) {
    if (value_equal(scope->locals[at].name, name)) {
      return at;
    }
  }
  return -1;
}

void scope_debug(struct scope *scope)
{
  printf("========== debug scope ==========\n");
  printf("sp: %d cur_depth: %d\n", scope->sp, scope->cur_depth);
  for (int i = 0; i <= scope->sp; i++) {
    printf("%5d depth %6d  ", i, scope->locals[i].depth);
    value_print(scope->locals[i].name);
    printf("\n");
  }
  printf("======== end debug scope ========\n");
}

// is_global returns true if the scope is in global env.
bool is_global(struct scope *scope) { return scope->cur_depth == 0; }

void compiler_init(struct compiler *compiler, char *src)
{
  lex_init(&compiler->lexer, src, strlen(src));
  scope_init(&compiler->scope);
  map_init(&compiler->mconstants);
  // initial forward
  forward(compiler);
}

void compiler_set_chunk(struct compiler *compiler, struct chunk *chunk)
{
  compiler->chunk = chunk;
}

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
  emit_bytes(compiler, OP_CONSTANT, make_constant(compiler, value));
}

static void emit_return(struct compiler *compiler)
{
  emit_byte(compiler, OP_RETURN);
}

// make_constant returns the idx of constant value. If the value has been
// created, return its idx. Else, add new constant to compiling chunk.
static uint8_t make_constant(struct compiler *compiler, struct value value)
{
#define value_as_int(value) ((int)value_as_number(value))
  struct value vidx;
  if (map_get(&compiler->mconstants, value, &vidx)) {
    return (uint8_t)value_as_int(vidx);
  }
  int constant = chunk_add_constant(compiler->chunk, value);
  if (constant > UINT8_MAX) {
    // FIXME: add error
    return 0;
  }
  vidx = value_make_number((double)constant);
  map_put(&compiler->mconstants, value, vidx);
  return (uint8_t)constant;
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
  if (token_type(&token) != type) {
    printf("expect %d but got %d", type, token.type);
    panic("");
  }
  return forward(compiler);
}

bool check(struct compiler *compiler, token_t type)
{
  return peek(compiler).type == type;
}

bool match(struct compiler *compiler, token_t type)
{
  if (check(compiler, type)) {
    forward(compiler);
    return true;
  }
  return false;
}

void compile(char *src, struct chunk *chunk)
{
  struct compiler compiler;
  compiler_init(&compiler, src);
  compiler_set_chunk(&compiler, chunk);

  while (!match(&compiler, TK_EOF)) {
    parse_decl(&compiler);
  }

  emit_byte(&compiler, OP_RETURN);
}

// CFG for program: [0/0]// program        → declaration* EOF ;
//
// declaration    → varDecl
//                → funDecl
//                | statement ;
//
// varDecl        → VAR IDENTIFIER "=" expression ;
//
// funDecl        → "fun" function ;
// function       → IDENTIFIER "(" parameters? ")" blockStmt ;
// parameters     → IDENTIFIER ("," IDENTIFIER)* ;
//
// statement      → exprStmt
//                | printStmt
//                | blockStmt
//                | ifStmt
//                | whileStmt
//                | forStmt
//                | returnStmt
//                | classStmt;
//
// exprStmt       → expression ";" ;
//
// printStmt      → "print" expression ";" ;
//
// blockStmt      → "{" declaration* "}" ;
//
// ifStmt         → "if" "(" expression ")" statement ("else" statement)? ;
//
// whileStmt      → "while" "(" expression ")" statement;
//
// forStmt        →
//   "for" "(" (varDecl | exprStmt | ";") expression? ";" expression? ")"
//   statement ;
//
// returnStmt     → "return" expression? ";" ;
//
// classStmt      → "class" IDENTIFIER "{" function* "}" ;

void parse_decl(struct compiler *compiler)
{
  if (match(compiler, TK_VAR)) {
    parse_var_decl(compiler);
  } else {
    parse_stmt(compiler);
  }
}

void defvar(struct compiler *compiler, struct value name)
{
  if (is_global(&compiler->scope)) {
    uint8_t constant = make_constant(compiler, name);
    emit_bytes(compiler, OP_GLOBAL, constant);
  } else {
    scope_add(&compiler->scope, name);
  }
}

// TODO: setvar and getvar have similar structure, consider refactor
void setvar(struct compiler *compiler, struct value name)
{
  int idx = scope_find(&compiler->scope, name);
  if (idx >= 0) {
    emit_bytes(compiler, OP_SET_LOCAL, (uint8_t)idx);
    return;
  }
  // find in globals
  uint8_t constant = make_constant(compiler, name);
  emit_bytes(compiler, OP_SET_GLOBAL, constant);
}

void getvar(struct compiler *compiler, struct value name)
{
  int idx = scope_find(&compiler->scope, name);
  if (idx >= 0) {
    emit_bytes(compiler, OP_GET_LOCAL, (uint8_t)idx);
    return;
  }
  // find in globals
  uint8_t constant = make_constant(compiler, name);
  emit_bytes(compiler, OP_GET_GLOBAL, constant);
}

static void eval(struct compiler *c, struct detail detail)
{
  if (detail.arity == 0) {
    return;
  }
  if (detail.id == TK_IDENT) {
    getvar(c, detail.first);
  } else {
    emit_constant(c, detail.first);
  }
}

void parse_var_decl(struct compiler *compiler)
{
  consume(compiler, TK_IDENT);
  struct token token = prev(compiler);
  struct value name
      = value_make_ident(token_lexem_start(&token), token_lexem_len(&token));

  if (match(compiler, TK_EQUAL)) {
    eval(compiler, expression(compiler, BP_NONE));
  } else {
    emit_constant(compiler, value_make_nil());
  }

  consume(compiler, TK_SEMICOLON);
  defvar(compiler, name);
}

void parse_stmt(struct compiler *compiler)
{
  if (match(compiler, TK_PRINT)) {
    return parse_print_stmt(compiler);
  } else if (match(compiler, TK_LEFT_BRACE)) {
    return parse_block(compiler);
  } else {
    return parse_expr_stmt(compiler);
  }
}

void parse_expr_stmt(struct compiler *compiler)
{
  eval(compiler, expression(compiler, BP_NONE));
  consume(compiler, TK_SEMICOLON);
  emit_byte(compiler, OP_POP);
}

void parse_print_stmt(struct compiler *compiler)
{
  eval(compiler, expression(compiler, BP_NONE));
  consume(compiler, TK_SEMICOLON);
  emit_byte(compiler, OP_PRINT);
}

void parse_block(struct compiler *compiler)
{
  scope_in(&compiler->scope);
  while (!check(compiler, TK_RIGHT_BRACE) && !check(compiler, TK_EOF)) {
    parse_decl(compiler);
  }
  consume(compiler, TK_RIGHT_BRACE);
  scope_out(&compiler->scope);
}

// Pratt parsing algorithm

struct symbol {
  nud_func nud;
  led_func led;
  binding_power bp;
  bool left_associative;
};

struct symbol symbols[TK_MAX + 1] = {
  // literals
  [TK_NIL] = { literal, NULL, BP_NONE, true },
  [TK_TRUE] = { literal, NULL, BP_NONE, true },
  [TK_FALSE] = { literal, NULL, BP_NONE, true },
  [TK_NUMBER] = { literal, NULL, BP_NONE, true },
  [TK_IDENT] = { variable, NULL, BP_NONE, true },
  [TK_STRING] = { literal, NULL, BP_NONE, true },

  // operators
  [TK_BANG] = { not, NULL, BP_NONE, true },

  // '-' has nud and led
  [TK_MINUS] = { negative, infix, BP_TERM, true },
  [TK_PLUS] = { NULL, infix, BP_TERM, true },

  [TK_SLASH] = { NULL, infix, BP_FACTOR, true },
  [TK_STAR] = { NULL, infix, BP_FACTOR, true },

  [TK_BANG_EQUAL] = { NULL, infix, BP_EQUALITY, true },
  [TK_EQUAL_EQUAL] = { NULL, infix, BP_EQUALITY, true },

  [TK_GREATER] = { NULL, infix, BP_COMPARISON, true },
  [TK_GREATER_EQUAL] = { NULL, infix, BP_COMPARISON, true },
  [TK_LESS] = { NULL, infix, BP_COMPARISON, true },
  [TK_LESS_EQUAL] = { NULL, infix, BP_COMPARISON, true },

  [TK_AND] = { NULL, infix, BP_AND, true },
  [TK_OR] = { NULL, infix, BP_OR, true },

  // '=' is right-associative
  [TK_EQUAL] = { NULL, assignment, BP_ASSIGNMENT, false },

  // '(' has nud
  [TK_LEFT_PAREN] = { group, NULL, BP_NONE, true },

  // others
  [TK_RIGHT_PAREN] = { NULL, NULL, BP_NONE, true },
  [TK_LEFT_BRACE] = { NULL, NULL, BP_NONE, true },
  [TK_RIGHT_BRACE] = { NULL, NULL, BP_NONE, true },
  [TK_COMMA] = { NULL, NULL, BP_NONE, true },
  [TK_DOT] = { NULL, NULL, BP_NONE, true },
  [TK_SEMICOLON] = { NULL, NULL, BP_NONE, true },

  // Keywords
  [TK_FOR] = { NULL, NULL, BP_NONE, true },
  [TK_PRINT] = { NULL, NULL, BP_NONE, true },
  [TK_RETURN] = { NULL, NULL, BP_NONE, true },
  [TK_CLASS] = { NULL, NULL, BP_NONE, true },
  [TK_THIS] = { NULL, NULL, BP_NONE, true },
  [TK_ELSE] = { NULL, NULL, BP_NONE, true },
  [TK_IF] = { NULL, NULL, BP_NONE, true },
  [TK_VAR] = { NULL, NULL, BP_NONE, true },
  [TK_WHILE] = { NULL, NULL, BP_NONE, true },
  [TK_FUN] = { NULL, NULL, BP_NONE, true },
  [TK_SUPER] = { NULL, NULL, BP_NONE, true },
};

static struct detail empty_detail(token_t id)
{
  struct detail detail = {
    .arity = 0,
    .id = id,
  };
  return detail;
}

static struct detail unary_detail(token_t id, struct value first)
{
  struct detail detail = {
    .arity = 1,
    .id = id,
    .first = first,
  };
  return detail;
}

struct detail expression(struct compiler *compiler, binding_power bp)
{
  nud_func nud = token_nud(forward(compiler));
  struct detail left;
  if (nud) {
    left = nud(compiler);
  }
  while (bp < token_bp(peek(compiler))) {
    led_func led = token_led(forward(compiler));
    assert(led);
    left = led(compiler, left);
  }
  return left;
}

nud_func token_nud(struct token token) { return symbols[token.type].nud; }
led_func token_led(struct token token) { return symbols[token.type].led; }
binding_power token_bp(struct token token) { return symbols[token.type].bp; }

struct detail literal(struct compiler *compiler)
{
  struct value v;
  struct token tk = prev(compiler);
  switch (tk.type) {
  case TK_NIL:
    v = value_make_nil();
    break;
  case TK_FALSE:
    v = value_make_bool(false);
    break;
  case TK_TRUE:
    v = value_make_bool(true);
    break;
  case TK_NUMBER:
    v = value_make_number(strtod(token_lexem_start(&tk), NULL));
    break;
  case TK_STRING:
    v = value_make_string(token_lexem_start(&tk) + 1, token_lexem_len(&tk) - 2);
    break;
  }
  return unary_detail(tk.type, v);
}

struct detail variable(struct compiler *compiler)
{
  struct token tk = prev(compiler);
  struct value name
      = value_make_ident(token_lexem_start(&tk), token_lexem_len(&tk));
  return unary_detail(tk.type, name);
}

struct detail negative(struct compiler *compiler)
{
  struct detail right = expression(compiler, BP_UNARY);
  eval(compiler, right);
  emit_byte(compiler, OP_NEGATIVE);
  return empty_detail(TK_MINUS);
}

struct detail not(struct compiler * compiler)
{
  struct detail right = expression(compiler, BP_UNARY);
  eval(compiler, right);
  emit_byte(compiler, OP_NOT);
  return empty_detail(TK_BANG);
}

struct detail group(struct compiler *compiler)
{
  struct detail grouped = expression(compiler, BP_NONE);
  consume(compiler, TK_RIGHT_PAREN);
  return grouped;
}

struct detail assignment(struct compiler *c, struct detail left)
{
  if (left.id != TK_IDENT) {
    panic("Invalid assignment.");
  }
  struct token tk = prev(c);
  struct detail right = expression(c, token_bp(tk) - 1);
  eval(c, right);
  setvar(c, left.first);
  return empty_detail(tk.type);
}

op_code infix_opcode(struct token token)
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

struct detail infix(struct compiler *compiler, struct detail left)
{
  eval(compiler, left);
  struct token tk = prev(compiler);
  struct detail right = expression(compiler, token_bp(tk));
  eval(compiler, right);
  emit_byte(compiler, infix_opcode(tk));
  return empty_detail(tk.type);
}
