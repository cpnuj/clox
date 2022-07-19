#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "value.h"

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

static inline struct token peek(struct compiler *c) { return c->curr; }

static inline struct token prev(struct compiler *c) { return c->prev; }

static struct token forward(struct compiler *c)
{
  c->prev = c->curr;
  c->curr = lex(&c->lexer);
  if (c->curr.type == TK_ERR) {
    error_at(c, c->curr, c->lexer.errmsg);
  }
  return c->prev;
}

static struct token consume(struct compiler *c, token_t type, char *msg)
{
  struct token tk = peek(c);
  if (tk.type == type) {
    return forward(c);
  }
  error_at(c, tk, msg);
}

static bool check(struct compiler *c, token_t type)
{
  return peek(c).type == type;
}

static bool match(struct compiler *c, token_t type)
{
  if (check(c, type)) {
    forward(c);
    return true;
  }
  return false;
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

static int add_constant(struct compiler *c, struct value value)
{
  value_array_write(c->constants, value);
  return c->constants->len - 1;
}

// make_constant returns the idx of constant value. If the value has been
// created, return its idx. Else, add new constant to compiling chunk.
static uint8_t make_constant(struct compiler *c, struct value value)
{
#define value_as_int(value) ((int)value_as_number(value))
  struct value vidx;
  if (map_get(&c->mconstants, value, &vidx)) {
    return (uint8_t)value_as_int(vidx);
  }
  int constant = add_constant(c, value);
  if (constant > UINT8_MAX) {
    // FIXME: add error
    return 0;
  }
  vidx = value_make_number((double)constant);
  map_put(&c->mconstants, value, vidx);
#undef value_as_int
  return (uint8_t)constant;
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

static void scope_init(struct compiler *c)
{
  c->scope.bp = -1;
  c->scope.sp = -1;
  c->scope.cur_depth = 0;
}

static void scope_in(struct compiler *c) { c->scope.cur_depth++; }

static void scope_out(struct compiler *c)
{
  struct scope *scope = &c->scope;
  while (scope->sp >= 0) {
    if (scope->locals[scope->sp].depth != scope->cur_depth) {
      break;
    }
    scope->sp--;
    emit_byte(c, OP_POP);
  }
  scope->cur_depth--;
}

static inline int scope_cursp(struct compiler *c) { return c->scope.sp; }

static inline int scope_curbp(struct compiler *c) { return c->scope.bp; }

static inline void scope_setbp(struct compiler *c, int bp) { c->scope.bp = bp; }

static int scope_find_cur(struct scope *scope, struct value name)
{
  for (int at = scope->sp; at > scope->bp; at--) {
    if (scope->locals[at].depth != scope->cur_depth) {
      return -1;
    }
    if (value_equal(scope->locals[at].name, name)) {
      return at - scope->bp;
    }
  }
  return -1;
}

static int scope_find_all(struct scope *scope, struct value name)
{
  for (int at = scope->sp; at > scope->bp; at--) {
    if (value_equal(scope->locals[at].name, name)) {
      return at - scope->bp;
    }
  }
  return -1;
}

#define FIND_ALL 1
#define FIND_CUR 2
static int scope_find(struct compiler *c, struct value name, int method)
{
  if (method == FIND_ALL) {
    return scope_find_all(&c->scope, name);
  } else {
    return scope_find_cur(&c->scope, name);
  }
}

static void scope_add(struct compiler *c, struct value name)
{
  struct scope *scope = &c->scope;
  scope->sp++;
  scope->locals[scope->sp].depth = scope->cur_depth;
  scope->locals[scope->sp].name = name;
}

static void scope_debug(struct scope *scope)
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
static bool is_global(struct scope *scope) { return scope->cur_depth == 0; }

static void defvar(struct compiler *c, struct value name)
{
  if (is_global(&c->scope)) {
    uint8_t constant = make_constant(c, name);
    emit_bytes(c, OP_GLOBAL, constant);
  } else {
    scope_add(c, name);
  }
}

// TODO: setvar and getvar have similar structure, consider refactor
static void setvar(struct compiler *c, struct value name)
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

static void getvar(struct compiler *c, struct value name)
{
  int idx = scope_find(c, name, FIND_ALL);
  if (idx >= 0) {
    emit_bytes(c, OP_GET_LOCAL, (uint8_t)idx);
    return;
  }
  // find in globals
  uint8_t constant = make_constant(c, name);
  emit_bytes(c, OP_GET_GLOBAL, constant);
}

// Pratt parsing algorithm

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

// struct context stores the parsing info of context expression,
// used by led_func.
// In our one-pass compiler, in general we parse a series of tokens
// and emit codes immediately. But resolving variables is an exception
// since we don't know we should set or get the value of the variable
// until we see the following tokens. So we store the information
// in this context structure, and operators should eval these details
// in their own way.
// Currently, only literal and variable would be evaled effectively.
// Nud functions of literal and variable would not emit any code
// until its context is evaled by the led of operators.
struct context {
  token_t id;
  int arity;
  struct value first;
};

static struct context empty_context(token_t id)
{
  struct context context = {
    .arity = 0,
    .id = id,
  };
  return context;
}

static struct context unary_context(token_t id, struct value first)
{
  struct context context = {
    .arity = 1,
    .id = id,
    .first = first,
  };
  return context;
}

static void eval(struct compiler *c, struct context context)
{
  if (context.arity == 0) {
    return;
  }
  if (context.id == TK_IDENT) {
    getvar(c, context.first);
  } else {
    emit_constant(c, context.first);
  }
}

// nud_func denotes operator with one operand
typedef struct context (*nud_func)(struct compiler *);
// led_func denotes operator with more than one operands
typedef struct context (*led_func)(struct compiler *, struct context);

struct symbol {
  nud_func nud;
  led_func led;
  binding_power bp;
};

static struct symbol symbols[TK_MAX + 1];

static inline int cur_pos(struct compiler *c) { return chunk_len(c->chunk); }

#define nud_of(token) (symbols[token.type].nud)
#define led_of(token) (symbols[token.type].led)
#define bp_of(token) (symbols[token.type].bp)

static struct context expression(struct compiler *c, binding_power bp)
{
  struct context left;
  nud_func nud = nud_of(forward(c));
  if (nud == NULL) {
    errorf(c, "Expect expression.");
    return empty_context(TK_ERR);
  }
  left = nud(c);
  while (bp < bp_of(peek(c))) {
    led_func led = led_of(forward(c));
    if (led == NULL) {
      errorf(c, "Expect expression.");
      return empty_context(TK_ERR);
    }
    left = led(c, left);
  }
  return left;
}

static struct context literal(struct compiler *c)
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
  return unary_context(tk.type, v);
}

static struct context variable(struct compiler *c)
{
  struct token tk = prev(c);
  struct value name
      = value_make_ident(token_lexem_start(&tk), token_lexem_len(&tk));
  return unary_context(tk.type, name);
}

static struct context negative(struct compiler *c)
{
  struct context right = expression(c, BP_UNARY);
  eval(c, right);
  emit_byte(c, OP_NEGATIVE);
  return empty_context(TK_MINUS);
}

static struct context not(struct compiler * c)
{
  struct context right = expression(c, BP_UNARY);
  eval(c, right);
  emit_byte(c, OP_NOT);
  return empty_context(TK_BANG);
}

static struct context group(struct compiler *c)
{
  struct context grouped = expression(c, BP_NONE);
  consume(c, TK_RIGHT_PAREN, "Expect ')' after group.");
  eval(c, grouped);
  return empty_context(TK_LEFT_PAREN);
}

static struct context ret(struct compiler *c)
{
  if (is_global(&c->scope)) {
    errorf(c, "Can't return from top-level code.");
  }
  if (!check(c, TK_SEMICOLON)) {
    eval(c, expression(c, BP_NONE));
  } else {
    emit_constant(c, value_make_nil());
  }
  emit_return(c);
  return empty_context(TK_RETURN);
}

static struct context assignment(struct compiler *c, struct context left)
{
  if (left.id != TK_IDENT) {
    errorf(c, "Invalid assignment target.");
    return empty_context(TK_ERR);
  }
  struct token tk = prev(c);
  struct context right = expression(c, bp_of(tk) - 1);
  eval(c, right);
  setvar(c, left.first);
  return empty_context(tk.type);
}

static op_code infix_opcode(struct token token)
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

static struct context infix(struct compiler *c, struct context left)
{
  eval(c, left);
  struct token tk = prev(c);
  struct context right = expression(c, bp_of(tk));
  eval(c, right);
  emit_byte(c, infix_opcode(tk));
  return empty_context(tk.type);
}

static struct context infix_and(struct compiler *c, struct context left)
{
  eval(c, left);
  int jmp_pos = emit_jmp(c, OP_JMP_ON_FALSE);
  emit_byte(c, OP_POP);
  eval(c, expression(c, BP_AND));
  patch_jmp(c, jmp_pos, cur_pos(c));
  return empty_context(TK_AND);
}

static struct context infix_or(struct compiler *c, struct context left)
{
  eval(c, left);
  int fail_pos = emit_jmp(c, OP_JMP_ON_FALSE);
  int succ_pos = emit_jmp(c, OP_JMP);
  patch_jmp(c, fail_pos, cur_pos(c));
  emit_byte(c, OP_POP);
  eval(c, expression(c, TK_AND));
  patch_jmp(c, succ_pos, cur_pos(c));
  return empty_context(TK_OR);
}

static struct context call(struct compiler *c, struct context left)
{
  eval(c, left);

  int arity = 0;
  if (!check(c, TK_RIGHT_PAREN)) {
    do {
      eval(c, expression(c, BP_NONE));
      arity++;
      if (arity > UINT8_MAX) {
        errorf(c, "Can't have more than 255 arguments.");
      }
    } while (match(c, TK_COMMA));
  }
  consume(c, TK_RIGHT_PAREN, "Expect ')' after arguments.");

  emit_bytes(c, OP_CALL, (uint8_t)arity);

  return empty_context(TK_LEFT_PAREN);
}

static void nud_symbol(token_t id, nud_func nud)
{
  symbols[id].bp = BP_NONE;
  symbols[id].nud = nud;
}

static void led_symbol(token_t id, binding_power bp, led_func led)
{
  symbols[id].bp = bp;
  symbols[id].led = led;
}

static void just_symbol(token_t id) { symbols[id].bp = BP_NONE; }

static void block_stmt(struct compiler *);
static void if_stmt(struct compiler *);
static void while_stmt(struct compiler *);
static void for_stmt(struct compiler *);
static void expr_stmt(struct compiler *);
static void print_stmt(struct compiler *);
static void statement(struct compiler *);
static void var_declaration(struct compiler *);
static void fun_declaration(struct compiler *);
static void declaration(struct compiler *);

static void block_stmt(struct compiler *c)
{
  scope_in(c);

  while (!check(c, TK_RIGHT_BRACE) && !check(c, TK_EOF)) {
    declaration(c);
    if (c->panic) {
      return;
    }
  }
  consume(c, TK_RIGHT_BRACE, "Expect '}' after block.");

  scope_out(c);
}

static void if_stmt(struct compiler *c)
{
  consume(c, TK_LEFT_PAREN, "Expect '(' after 'if'.");
  eval(c, expression(c, BP_NONE));
  consume(c, TK_RIGHT_PAREN, "Expect ')' after condition.");

  int jmp_pos = emit_jmp(c, OP_JMP_ON_FALSE);
  emit_byte(c, OP_POP);
  statement(c);

  int then_jmp_pos = emit_jmp(c, OP_JMP);
  patch_jmp(c, jmp_pos, cur_pos(c));
  emit_byte(c, OP_POP);
  if (match(c, TK_ELSE)) {
    statement(c);
  }
  patch_jmp(c, then_jmp_pos, cur_pos(c));
}

static void while_stmt(struct compiler *c)
{
  // Record current pos for jumping back
  int cond_pos = cur_pos(c);

  consume(c, TK_LEFT_PAREN, "Expect '(' after 'while'.");
  eval(c, expression(c, BP_NONE));
  consume(c, TK_RIGHT_PAREN, "Expect ')' after condition.");

  int jmp_pos = emit_jmp(c, OP_JMP_ON_FALSE);
  emit_byte(c, OP_POP); // pop out op_jmp_on_false
  statement(c);
  patch_jmp(c, emit_jmp(c, OP_JMP_BACK), cond_pos);

  patch_jmp(c, jmp_pos, cur_pos(c));
  emit_byte(c, OP_POP); // pop out op_jmp_on_false
}

static void for_stmt(struct compiler *c)
{
  scope_in(c);

  consume(c, TK_LEFT_PAREN, "Expect '(' after 'for'.");

  // initializer
  if (match(c, TK_SEMICOLON)) {
    // do nothing
  } else if (match(c, TK_VAR)) {
    var_declaration(c);
  } else {
    expr_stmt(c);
  }

  // condition
  int cond_pos = cur_pos(c);
  if (check(c, TK_SEMICOLON)) {
    emit_constant(c, value_make_bool(true));
  } else {
    eval(c, expression(c, BP_NONE));
  }
  consume(c, TK_SEMICOLON, "Expect ';' after condition.");

  int jmp_when_fail = emit_jmp(c, OP_JMP_ON_FALSE);
  emit_byte(c, OP_POP);
  int jmp_to_body = emit_jmp(c, OP_JMP);

  // incrementer
  int inc_pos = cur_pos(c);
  if (!match(c, TK_RIGHT_PAREN)) {
    eval(c, expression(c, BP_NONE));
    emit_byte(c, OP_POP);
    consume(c, TK_RIGHT_PAREN, "Expect ')' after incrementer.");
  }
  patch_jmp(c, emit_jmp(c, OP_JMP_BACK), cond_pos);

  // body
  patch_jmp(c, jmp_to_body, cur_pos(c));
  statement(c);
  // jump to incrementer
  patch_jmp(c, emit_jmp(c, OP_JMP_BACK), inc_pos);

  patch_jmp(c, jmp_when_fail, cur_pos(c));
  emit_byte(c, OP_POP);

  scope_out(c);
}

static void expr_stmt(struct compiler *c)
{
  eval(c, expression(c, BP_NONE));
  consume(c, TK_SEMICOLON, "Expect ';' after expression declaration.");
  emit_byte(c, OP_POP);
}

static void print_stmt(struct compiler *c)
{
  eval(c, expression(c, BP_NONE));
  consume(c, TK_SEMICOLON, "Expect ';' after print declaration.");
  emit_byte(c, OP_PRINT);
}

static void statement(struct compiler *c)
{
  if (match(c, TK_PRINT)) {
    return print_stmt(c);
  } else if (match(c, TK_LEFT_BRACE)) {
    return block_stmt(c);
  } else if (match(c, TK_IF)) {
    return if_stmt(c);
  } else if (match(c, TK_WHILE)) {
    return while_stmt(c);
  } else if (match(c, TK_FOR)) {
    return for_stmt(c);
  } else {
    return expr_stmt(c);
  }
}

static void var_declaration(struct compiler *c)
{
  consume(c, TK_IDENT, "Expect variable name.");
  struct context ctx = variable(c);
  struct value name = ctx.first;

  if (scope_find(c, name, FIND_CUR) != -1) {
    errorf(c, "Already a variable with this name in this scope.");
    return;
  }

  if (match(c, TK_EQUAL)) {
    struct context initializer = expression(c, BP_NONE);

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

static void fun_declaration(struct compiler *c)
{
  consume(c, TK_IDENT, "Expect function name.");
  struct value fname = variable(c).first;

  if (scope_find(c, fname, FIND_CUR) != -1) {
    errorf(c, "Already a variable with this name in this scope.");
    return;
  }

  consume(c, TK_LEFT_PAREN, "Expect '(' after function name.");

  // parse parameters
  int arity = 0;
  struct value paras[255];
  while (match(c, TK_IDENT)) {
    if (arity >= 255) {
      errorf(c, "Can't have more than 255 parameters.");
      return;
    }
    paras[arity] = variable(c).first;
    arity++;
    if (!match(c, TK_COMMA)) {
      break;
    }
  }

  consume(c, TK_RIGHT_PAREN, "Expect ')' after parameters.");

  struct value fun = value_make_fun(arity, value_as_string(fname));
  emit_constant(c, fun);
  defvar(c, fname);

  // enter new scope and set scope base pointer
  int oldbp = scope_curbp(c);
  scope_setbp(c, scope_cursp(c));
  scope_in(c);
  defvar(c, fname);
  for (int i = 0; i < arity; i++) {
    defvar(c, paras[i]);
  }

  consume(c, TK_LEFT_BRACE, "Expect '{' before function body.");

  // switch compiling chunk
  struct chunk *enclosing = c->chunk;
  struct obj_fun *funobj = (struct obj_fun *)value_as_obj(fun);
  c->chunk = &funobj->chunk;

  block_stmt(c);
  emit_constant(c, value_make_nil());
  emit_byte(c, OP_RETURN);

#ifdef DEBUG
  debug_chunk(c->chunk, c->constants, value_as_string(fname)->str);
#endif

  scope_out(c);
  scope_setbp(c, oldbp);

  // back to previous compiling chunk
  c->chunk = enclosing;
}

static void declaration(struct compiler *c)
{
  if (match(c, TK_VAR)) {
    var_declaration(c);
  } else if (match(c, TK_FUN)) {
    fun_declaration(c);
  } else {
    statement(c);
  }
}

static void setup()
{
  static bool done = false;
  if (done)
    return;

  // literals
  nud_symbol(TK_NIL, literal);
  nud_symbol(TK_TRUE, literal);
  nud_symbol(TK_FALSE, literal);
  nud_symbol(TK_NUMBER, literal);
  nud_symbol(TK_STRING, literal);
  nud_symbol(TK_IDENT, variable);

  // operators
  nud_symbol(TK_BANG, not );

  // '-' has nud and led
  nud_symbol(TK_MINUS, negative);
  led_symbol(TK_MINUS, BP_TERM, infix);
  led_symbol(TK_PLUS, BP_TERM, infix);

  led_symbol(TK_SLASH, BP_FACTOR, infix);
  led_symbol(TK_STAR, BP_FACTOR, infix);

  led_symbol(TK_BANG_EQUAL, BP_EQUALITY, infix);
  led_symbol(TK_EQUAL_EQUAL, BP_EQUALITY, infix);

  led_symbol(TK_GREATER, BP_COMPARISON, infix);
  led_symbol(TK_GREATER_EQUAL, BP_COMPARISON, infix);
  led_symbol(TK_LESS, BP_COMPARISON, infix);
  led_symbol(TK_LESS_EQUAL, BP_COMPARISON, infix);

  led_symbol(TK_AND, BP_AND, infix_and);
  led_symbol(TK_OR, BP_OR, infix_or);

  led_symbol(TK_EQUAL, BP_ASSIGNMENT, assignment);

  // '(' has nud
  nud_symbol(TK_LEFT_PAREN, group);
  led_symbol(TK_LEFT_PAREN, BP_CALL, call);

  // others
  just_symbol(TK_RIGHT_PAREN);
  just_symbol(TK_LEFT_BRACE);
  just_symbol(TK_RIGHT_BRACE);
  just_symbol(TK_COMMA);
  just_symbol(TK_DOT);
  just_symbol(TK_SEMICOLON);

  // Keywords
  just_symbol(TK_FOR);
  just_symbol(TK_PRINT);

  nud_symbol(TK_RETURN, ret);

  just_symbol(TK_CLASS);
  just_symbol(TK_THIS);
  just_symbol(TK_ELSE);
  just_symbol(TK_IF);
  just_symbol(TK_VAR);
  just_symbol(TK_WHILE);
  just_symbol(TK_FUN);
  just_symbol(TK_SUPER);

  // eof
  just_symbol(TK_EOF);
}

static void compiler_init(struct compiler *c, char *src)
{
  setup();

  c->panic = 0;
  c->error = 0;

  lex_init(&c->lexer, src, strlen(src));
  scope_init(c);
  scope_add(c, value_make_string("main", 4));
  map_init(&c->mconstants);

  // initial forward
  forward(c);
}

static int compile_chunk(char *src, struct chunk *chunk,
                         struct value_list *constants)
{
  struct compiler c;
  compiler_init(&c, src);
  c.chunk = chunk;
  c.constants = constants;

  while (!match(&c, TK_EOF)) {
    if (c.panic) {
      break;
    }
    declaration(&c);
  }
  emit_constant(&c, value_make_nil());
  emit_byte(&c, OP_RETURN);

  return c.error;
}

int compile(char *src, struct obj_fun *fun, struct value_list *constants)
{
  int err = compile_chunk(src, &fun->chunk, constants);

#ifdef DEBUG
  debug_chunk(&fun->chunk, constants, fun->name->str);
#endif

  return err;
}