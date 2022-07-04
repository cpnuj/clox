#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "value.h"

void scope_init(struct scope *);
void scope_in(struct scope *);
int scope_out(struct scope *);
void scope_add(struct scope *, struct value);
int scope_find(struct scope *, struct value, int);
int scope_find_cur(struct scope *, struct value);
int scope_find_all(struct scope *, struct value);
void scope_debug(struct scope *);
bool is_global(struct scope *);

void compiler_init(struct compiler *, char *);
void compiler_set_chunk(struct compiler *, struct chunk *);
static void error_at(struct compiler *, struct token, char *);
static void errorf(struct compiler *, char *, ...);

static void emit_byte(struct compiler *, uint8_t);
static void emit_bytes(struct compiler *, uint8_t, uint8_t);
static void emit_constant(struct compiler *, struct value);
static int emit_jmp(struct compiler *, uint8_t);
static void patch_jmp(struct compiler *, int, int);
static void emit_return(struct compiler *);

static uint8_t make_constant(struct compiler *, struct value);

struct token peek(struct compiler *);
struct token prev(struct compiler *);
struct token forward(struct compiler *);
struct token consume(struct compiler *, token_t, char *);
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

struct detail expression(struct compiler *c, binding_power bp);
struct detail literal(struct compiler *c);
struct detail variable(struct compiler *c);
struct detail negative(struct compiler *c);
struct detail not(struct compiler * c);
struct detail group(struct compiler *c);
struct detail assignment(struct compiler *c, struct detail left);
struct detail infix_and(struct compiler *c, struct detail left);
struct detail infix_or(struct compiler *c, struct detail left);
struct detail infix(struct compiler *c, struct detail left);
op_code infix_opcode(struct token token);

void declaration(struct compiler *c);
void var_declaration(struct compiler *c);
void statement(struct compiler *c);
void expr_stmt(struct compiler *c);
void print_stmt(struct compiler *c);
void block_stmt(struct compiler *c);
void if_stmt(struct compiler *c);

void defvar(struct compiler *c, struct value name);
void setvar(struct compiler *c, struct value name);
void getvar(struct compiler *c, struct value name);
static void eval(struct compiler *, struct detail);

void scope_init(struct scope *scope)
{
  scope->sp = -1;
  scope->cur_depth = 0;
}

void scope_in(struct scope *scope) { scope->cur_depth++; }

int scope_out(struct scope *scope)
{
  // pop all locals belonging to current depth
  int poped = 0;
  while (scope->sp >= 0) {
    if (scope->locals[scope->sp].depth != scope->cur_depth) {
      break;
    }
    scope->sp--;
    poped++;
  }
  scope->cur_depth--;
  return poped;
}

void scope_add(struct scope *scope, struct value name)
{
  scope->sp++;
  scope->locals[scope->sp].depth = scope->cur_depth;
  scope->locals[scope->sp].name = name;
}

#define FIND_ALL 1
#define FIND_CUR 2
int scope_find(struct scope *scope, struct value name, int method)
{
  if (method == FIND_ALL) {
    return scope_find_all(scope, name);
  } else {
    return scope_find_cur(scope, name);
  }
}

int scope_find_cur(struct scope *scope, struct value name)
{
  for (int at = scope->sp; at >= 0; at--) {
    if (scope->locals[at].depth != scope->cur_depth) {
      return -1;
    }
    if (value_equal(scope->locals[at].name, name)) {
      return at;
    }
  }
  return -1;
}

int scope_find_all(struct scope *scope, struct value name)
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

void compiler_init(struct compiler *c, char *src)
{
  c->panic = 0;
  c->error = 0;

  lex_init(&c->lexer, src, strlen(src));
  scope_init(&c->scope);
  map_init(&c->mconstants);

  // initial forward
  forward(c);
}

void compiler_set_chunk(struct compiler *c, struct chunk *chunk)
{
  c->chunk = chunk;
}

static void error_at(struct compiler *c, struct token tk, char *msg)
{
  if (c->panic)
    return;

  c->panic = 1;

  if (tk.type == TK_EOF) {
    fprintf(stderr, "[line %d] Error at end: %s\n", tk.line, msg);
  } else if (tk.type == TK_ERR) {
    fprintf(stderr, "%s\n", msg);
  } else {
    fprintf(stderr, "[line %d] Error at '%.*s': %s\n", tk.line, tk.len, tk.at,
            msg);
  }

  c->error = 1;
}

static void errorf(struct compiler *c, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vsprintf(c->errmsg, fmt, ap);
  error_at(c, prev(c), c->errmsg);
}

static void emit_byte(struct compiler *c, uint8_t byte)
{
  chunk_add(c->chunk, byte, c->prev.line);
}

static void emit_bytes(struct compiler *c, uint8_t b1, uint8_t b2)
{
  emit_byte(c, b1);
  emit_byte(c, b2);
}

static void emit_constant(struct compiler *c, struct value value)
{
  emit_bytes(c, OP_CONSTANT, make_constant(c, value));
}

static int emit_jmp(struct compiler *c, uint8_t jmpop)
{
  int jmpat = chunk_len(c->chunk);
  emit_byte(c, jmpop);
  emit_bytes(c, 0, 0);
  return jmpat;
}

static void patch_jmp(struct compiler *c, int jmp_pos, int target_pos)
{
  int offset;
  if (target_pos >= jmp_pos) {
    offset = target_pos - (jmp_pos + 3);
  } else {
    offset = jmp_pos + 3 - target_pos;
  }
  chunk_set(c->chunk, jmp_pos + 1, (offset >> 8) & 0xff);
  chunk_set(c->chunk, jmp_pos + 2, (offset)&0xff);
}

static void emit_return(struct compiler *c) { emit_byte(c, OP_RETURN); }

// make_constant returns the idx of constant value. If the value has been
// created, return its idx. Else, add new constant to compiling chunk.
static uint8_t make_constant(struct compiler *c, struct value value)
{
#define value_as_int(value) ((int)value_as_number(value))
  struct value vidx;
  if (map_get(&c->mconstants, value, &vidx)) {
    return (uint8_t)value_as_int(vidx);
  }
  int constant = chunk_add_constant(c->chunk, value);
  if (constant > UINT8_MAX) {
    // FIXME: add error
    return 0;
  }
  vidx = value_make_number((double)constant);
  map_put(&c->mconstants, value, vidx);
  return (uint8_t)constant;
}

struct token peek(struct compiler *c) { return c->curr; }

struct token prev(struct compiler *c) { return c->prev; }

struct token forward(struct compiler *c)
{
  c->prev = c->curr;
  c->curr = lex(&c->lexer);
  if (c->curr.type == TK_ERR) {
    error_at(c, c->curr, c->lexer.errmsg);
  }
  return c->prev;
}

struct token consume(struct compiler *c, token_t type, char *msg)
{
  struct token tk = peek(c);
  if (tk.type == type) {
    return forward(c);
  }
  error_at(c, tk, msg);
}

bool check(struct compiler *c, token_t type) { return peek(c).type == type; }

bool match(struct compiler *c, token_t type)
{
  if (check(c, type)) {
    forward(c);
    return true;
  }
  return false;
}

void defvar(struct compiler *c, struct value name)
{
  if (is_global(&c->scope)) {
    uint8_t constant = make_constant(c, name);
    emit_bytes(c, OP_GLOBAL, constant);
  } else {
    scope_add(&c->scope, name);
  }
}

// TODO: setvar and getvar have similar structure, consider refactor
void setvar(struct compiler *c, struct value name)
{
  int idx = scope_find_all(&c->scope, name);
  if (idx >= 0) {
    emit_bytes(c, OP_SET_LOCAL, (uint8_t)idx);
    return;
  }
  // find in globals
  uint8_t constant = make_constant(c, name);
  emit_bytes(c, OP_SET_GLOBAL, constant);
}

void getvar(struct compiler *c, struct value name)
{
  int idx = scope_find(&c->scope, name, FIND_ALL);
  if (idx >= 0) {
    emit_bytes(c, OP_GET_LOCAL, (uint8_t)idx);
    return;
  }
  // find in globals
  uint8_t constant = make_constant(c, name);
  emit_bytes(c, OP_GET_GLOBAL, constant);
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

int compile(char *src, struct chunk *chunk)
{
  struct compiler c;
  compiler_init(&c, src);
  compiler_set_chunk(&c, chunk);

  while (!match(&c, TK_EOF)) {
    if (c.panic) {
      break;
    }
    declaration(&c);
  }

  emit_byte(&c, OP_RETURN);
  return c.error;
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

void declaration(struct compiler *c)
{
  if (match(c, TK_VAR)) {
    var_declaration(c);
  } else {
    statement(c);
  }
}

void var_declaration(struct compiler *c)
{
  consume(c, TK_IDENT, "Expect variable name.");
  struct token token = prev(c);
  struct value name
      = value_make_ident(token_lexem_start(&token), token_lexem_len(&token));

  if (scope_find(&c->scope, name, FIND_CUR) != -1) {
    errorf(c, "Already a variable with this name in this scope.");
    return;
  }

  if (match(c, TK_EQUAL)) {
    struct detail initializer = expression(c, BP_NONE);

    bool in_local = !is_global(&c->scope);
    bool is_own_initializer
        = initializer.id == TK_IDENT && value_equal(initializer.first, name);

    if (in_local && is_own_initializer) {
      errorf(c, "Can't read local variable in its own initializer.");
      return;
    }

    eval(c, initializer);
  } else {
    emit_constant(c, value_make_nil());
  }

  defvar(c, name);

  consume(c, TK_SEMICOLON, "Expect ';' after variable declaration.");
}

void statement(struct compiler *c)
{
  if (match(c, TK_PRINT)) {
    return print_stmt(c);
  } else if (match(c, TK_LEFT_BRACE)) {
    return block_stmt(c);
  } else if (match(c, TK_IF)) {
    return if_stmt(c);
  } else {
    return expr_stmt(c);
  }
}

void expr_stmt(struct compiler *c)
{
  eval(c, expression(c, BP_NONE));
  consume(c, TK_SEMICOLON, "Expect ';' after expression declaration.");
  emit_byte(c, OP_POP);
}

void print_stmt(struct compiler *c)
{
  eval(c, expression(c, BP_NONE));
  consume(c, TK_SEMICOLON, "Expect ';' after print declaration.");
  emit_byte(c, OP_PRINT);
}

void block_stmt(struct compiler *c)
{
  scope_in(&c->scope);

  while (!check(c, TK_RIGHT_BRACE) && !check(c, TK_EOF)) {
    declaration(c);
    if (c->panic) {
      return;
    }
  }
  consume(c, TK_RIGHT_BRACE, "Expect '}' after block.");

  int poped = scope_out(&c->scope);
  // Synchronize run-time stack with scope.
  for (int i = 0; i < poped; i++) {
    emit_byte(c, OP_POP);
  }
}

static inline int cur_pos(struct compiler *c) { return chunk_len(c->chunk); }

void if_stmt(struct compiler *c)
{
  consume(c, TK_LEFT_PAREN, "Expect '(' after 'if'.");
  eval(c, expression(c, BP_NONE));
  consume(c, TK_RIGHT_PAREN, "Expect ')' after condition.");

  int jmp_pos = emit_jmp(c, OP_JMP_ON_FALSE);
  emit_byte(c, OP_POP);
  statement(c);

  if (match(c, TK_ELSE)) {
    int then_jmp_pos = emit_jmp(c, OP_JMP);
    patch_jmp(c, jmp_pos, cur_pos(c));
    emit_byte(c, OP_POP);
    statement(c);
    patch_jmp(c, then_jmp_pos, cur_pos(c));
  } else {
    patch_jmp(c, jmp_pos, cur_pos(c));
  }
}

// Pratt parsing algorithm

struct symbol {
  nud_func nud;
  led_func led;
  binding_power bp;
};

struct symbol symbols[TK_MAX + 1] = {
  // literals
  [TK_NIL] = { literal, NULL, BP_NONE },
  [TK_TRUE] = { literal, NULL, BP_NONE },
  [TK_FALSE] = { literal, NULL, BP_NONE },
  [TK_NUMBER] = { literal, NULL, BP_NONE },
  [TK_IDENT] = { variable, NULL, BP_NONE },
  [TK_STRING] = { literal, NULL, BP_NONE },

  // operators
  [TK_BANG] = { not, NULL, BP_NONE },

  // '-' has nud and led
  [TK_MINUS] = { negative, infix, BP_TERM },
  [TK_PLUS] = { NULL, infix, BP_TERM },

  [TK_SLASH] = { NULL, infix, BP_FACTOR },
  [TK_STAR] = { NULL, infix, BP_FACTOR },

  [TK_BANG_EQUAL] = { NULL, infix, BP_EQUALITY },
  [TK_EQUAL_EQUAL] = { NULL, infix, BP_EQUALITY },

  [TK_GREATER] = { NULL, infix, BP_COMPARISON },
  [TK_GREATER_EQUAL] = { NULL, infix, BP_COMPARISON },
  [TK_LESS] = { NULL, infix, BP_COMPARISON },
  [TK_LESS_EQUAL] = { NULL, infix, BP_COMPARISON },

  [TK_AND] = { NULL, infix_and, BP_AND },
  [TK_OR] = { NULL, infix_or, BP_OR },

  [TK_EQUAL] = { NULL, assignment, BP_ASSIGNMENT },

  // '(' has nud
  [TK_LEFT_PAREN] = { group, NULL, BP_NONE },

  // others
  [TK_RIGHT_PAREN] = { NULL, NULL, BP_NONE },
  [TK_LEFT_BRACE] = { NULL, NULL, BP_NONE },
  [TK_RIGHT_BRACE] = { NULL, NULL, BP_NONE },
  [TK_COMMA] = { NULL, NULL, BP_NONE },
  [TK_DOT] = { NULL, NULL, BP_NONE },
  [TK_SEMICOLON] = { NULL, NULL, BP_NONE },

  // Keywords
  [TK_FOR] = { NULL, NULL, BP_NONE },
  [TK_PRINT] = { NULL, NULL, BP_NONE },
  [TK_RETURN] = { NULL, NULL, BP_NONE },
  [TK_CLASS] = { NULL, NULL, BP_NONE },
  [TK_THIS] = { NULL, NULL, BP_NONE },
  [TK_ELSE] = { NULL, NULL, BP_NONE },
  [TK_IF] = { NULL, NULL, BP_NONE },
  [TK_VAR] = { NULL, NULL, BP_NONE },
  [TK_WHILE] = { NULL, NULL, BP_NONE },
  [TK_FUN] = { NULL, NULL, BP_NONE },
  [TK_SUPER] = { NULL, NULL, BP_NONE },

  // eof
  [TK_EOF] = { NULL, NULL, BP_NONE },
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

struct detail expression(struct compiler *c, binding_power bp)
{
  struct detail left;
  nud_func nud = token_nud(forward(c));
  if (nud == NULL) {
    errorf(c, "Expect expression.");
    return empty_detail(TK_ERR);
  }
  left = nud(c);
  while (bp < token_bp(peek(c))) {
    led_func led = token_led(forward(c));
    if (led == NULL) {
      errorf(c, "Expect expression.");
      return empty_detail(TK_ERR);
    }
    left = led(c, left);
  }
  return left;
}

nud_func token_nud(struct token token) { return symbols[token.type].nud; }
led_func token_led(struct token token) { return symbols[token.type].led; }
binding_power token_bp(struct token token) { return symbols[token.type].bp; }

struct detail literal(struct compiler *c)
{
  struct value v;
  struct token tk = prev(c);
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

struct detail variable(struct compiler *c)
{
  struct token tk = prev(c);
  struct value name
      = value_make_ident(token_lexem_start(&tk), token_lexem_len(&tk));
  return unary_detail(tk.type, name);
}

struct detail negative(struct compiler *c)
{
  struct detail right = expression(c, BP_UNARY);
  eval(c, right);
  emit_byte(c, OP_NEGATIVE);
  return empty_detail(TK_MINUS);
}

struct detail not(struct compiler * c)
{
  struct detail right = expression(c, BP_UNARY);
  eval(c, right);
  emit_byte(c, OP_NOT);
  return empty_detail(TK_BANG);
}

struct detail group(struct compiler *c)
{
  struct detail grouped = expression(c, BP_NONE);
  consume(c, TK_RIGHT_PAREN, "Expect ')' after group.");
  eval(c, grouped);
  return empty_detail(TK_LEFT_PAREN);
}

struct detail assignment(struct compiler *c, struct detail left)
{
  if (left.id != TK_IDENT) {
    errorf(c, "Invalid assignment target.");
    return empty_detail(TK_ERR);
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
  default:
    return OP_NONE;
  }
}

struct detail infix(struct compiler *c, struct detail left)
{
  eval(c, left);
  struct token tk = prev(c);
  struct detail right = expression(c, token_bp(tk));
  eval(c, right);
  emit_byte(c, infix_opcode(tk));
  return empty_detail(tk.type);
}

struct detail infix_and(struct compiler *c, struct detail left)
{
  eval(c, left);
  int jmp_pos = emit_jmp(c, OP_JMP_ON_FALSE);
  emit_byte(c, OP_POP);
  eval(c, expression(c, BP_AND));
  patch_jmp(c, jmp_pos, cur_pos(c));
  return empty_detail(TK_AND);
}

struct detail infix_or(struct compiler *c, struct detail left)
{
  eval(c, left);
  int fail_pos = emit_jmp(c, OP_JMP_ON_FALSE);
  int succ_pos = emit_jmp(c, OP_JMP);
  patch_jmp(c, fail_pos, cur_pos(c));
  emit_byte(c, OP_POP);
  eval(c, expression(c, TK_AND));
  patch_jmp(c, succ_pos, cur_pos(c));
  return empty_detail(TK_OR);
}
