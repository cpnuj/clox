#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include "value.h"

static void error_at(Compiler *c, Token tk, char *msg)
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

static inline Token peek(Compiler *c) { return c->curr; }

static inline Token prev(Compiler *c) { return c->prev; }

static Token forward(Compiler *c)
{
  c->prev = c->curr;
  c->curr = lex(&c->lexer);
  if (c->curr.type == TK_ERR) {
    error_at(c, c->curr, c->lexer.errmsg);
  }
  return c->prev;
}

static Token consume(Compiler *c, token_t type, char *msg)
{
  Token tk = peek(c);
  if (tk.type == type) {
    return forward(c);
  }
  error_at(c, tk, msg);
}

static bool check(Compiler *c, token_t type) { return peek(c).type == type; }

static bool match(Compiler *c, token_t type)
{
  if (check(c, type)) {
    forward(c);
    return true;
  }
  return false;
}

static void errorf(Compiler *c, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vsprintf(c->errmsg, fmt, ap);
  error_at(c, prev(c), c->errmsg);
}

static void emit_byte(Compiler *c, uint8_t byte)
{
  chunk_add(c->cur_chunk, byte, c->prev.line);
}

static void emit_bytes(Compiler *c, uint8_t b1, uint8_t b2)
{
  emit_byte(c, b1);
  emit_byte(c, b2);
}

static int add_constant(Compiler *c, Value value)
{
  value_array_write(c->constants, value);
  return c->constants->len - 1;
}

static Value make_string(Compiler *c, const char *src, int len)
{
  Value ret = value_make_nil();

  MapIter *iter = map_iter_new(&c->interned_strings);
  while (map_iter_next(iter)) {
    if (len != as_string(iter->key)->len) {
      continue;
    }
    if (strncmp(src, as_string(iter->key)->str, len) == 0) {
      ret = iter->key;
      break;
    }
  }
  map_iter_close(iter);

  if (is_nil(ret)) {
    ret = value_make_object(string_copy(src, len));
    map_put(&c->interned_strings, ret, value_make_nil());
  }

  return ret;
}

// make_constant returns the idx of constant value. If the value has been
// created, return its idx. Else, add new constant to compiling chunk.
static uint8_t make_constant(Compiler *c, Value value)
{
#define value_as_int(value) ((int)as_number(value))
  Value vidx;
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

static void emit_constant(Compiler *c, Value value)
{
  emit_bytes(c, OP_CONSTANT, make_constant(c, value));
}

static int emit_jmp(Compiler *c, uint8_t jmpop)
{
  int jmpat = chunk_len(c->cur_chunk);
  emit_byte(c, jmpop);
  emit_bytes(c, 0, 0);
  return jmpat;
}

static void patch_jmp(Compiler *c, int jmp_pos, int target_pos)
{
  int offset;
  if (target_pos >= jmp_pos) {
    offset = target_pos - (jmp_pos + 3);
  } else {
    offset = jmp_pos + 3 - target_pos;
  }
  chunk_set(c->cur_chunk, jmp_pos + 1, (offset >> 8) & 0xff);
  chunk_set(c->cur_chunk, jmp_pos + 2, (offset)&0xff);
}

static void emit_return(Compiler *c) { emit_byte(c, OP_RETURN); }

static void scope_init(Scope *scope)
{
  scope->sp = -1;
  scope->cur_depth = 0;
  scope->enclosing = NULL;
}

static void scope_in(Scope *scope) { scope->cur_depth++; }

static void scope_out(Scope *scope, Compiler *c)
{
  while (scope->sp >= 0) {
    if (scope->locals[scope->sp].depth != scope->cur_depth) {
      break;
    }
    if (scope->locals[scope->sp].is_captured) {
      emit_byte(c, OP_CLOSE);
    } else {
      emit_byte(c, OP_POP);
    }
    scope->sp--;
  }
  scope->cur_depth--;
}

static int scope_find_cur(Scope *scope, Value name)
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

static int scope_find_all(Scope *scope, Value name)
{
  for (int at = scope->sp; at >= 0; at--) {
    if (value_equal(scope->locals[at].name, name)) {
      return at;
    }
  }
  return -1;
}

#define FIND_ALL 1
#define FIND_CUR 2
static int find_local(Scope *scope, Value name, int method)
{
  if (method == FIND_ALL) {
    return scope_find_all(scope, name);
  } else {
    return scope_find_cur(scope, name);
  }
}

static void scope_add(Scope *scope, Value name)
{
  scope->sp++;
  scope->locals[scope->sp].depth = scope->cur_depth;
  scope->locals[scope->sp].name = name;
  scope->locals[scope->sp].is_captured = false;
}

static void scope_debug(Scope *scope)
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

static int find_upvalue(Scope *scope, Value name)
{
  for (int i = 0; i < scope->upvalue_size; i++) {
    if (value_equal(name, scope->upvalues[i].name)) {
      return i;
    }
  }
  return -1;
}

static int add_upvalue(Scope *scope, UpValue upvalue)
{
  assert(scope->upvalue_size <= UINT8_MAX);
  scope->upvalues[scope->upvalue_size++] = upvalue;
  return scope->upvalue_size - 1;
}

static int capture_upvalue(Scope *scope, Value name)
{
  if (scope->enclosing == NULL) {
    return -1;
  }

  int idx;

  idx = find_upvalue(scope, name);
  if (idx != -1) {
    return idx;
  }

  UpValue upvalue;
  upvalue.name = name;

  idx = find_local(scope->enclosing, name, FIND_ALL);
  if (idx != -1) {
    scope->enclosing->locals[idx].is_captured = true;
    upvalue.idx = idx;
    upvalue.from_local = true;
    return add_upvalue(scope, upvalue);
  }

  idx = capture_upvalue(scope->enclosing, name);
  if (idx != -1) {
    upvalue.idx = idx;
    upvalue.from_local = false;
    return add_upvalue(scope, upvalue);
  }

  return -1;
}

static void frame_enter(Compiler *c, Scope *new_scope)
{
  new_scope->enclosing = c->cur_scope;
  c->cur_scope = new_scope;
}

static void frame_out(Compiler *c) { c->cur_scope = c->cur_scope->enclosing; }

// is_global returns true if the scope is in global env.
static inline bool is_global(Scope *scope)
{
  return scope->enclosing == NULL && scope->cur_depth == 0;
}

static void defvar(Compiler *c, Value name)
{
  if (is_global(c->cur_scope)) {
    uint8_t constant = make_constant(c, name);
    emit_bytes(c, OP_GLOBAL, constant);
  } else {
    scope_add(c->cur_scope, name);
  }
}

// TODO: setvar and getvar have similar structure, consider refactor
static void setvar(Compiler *c, Value name)
{
  int idx;

  idx = find_local(c->cur_scope, name, FIND_ALL);
  if (idx >= 0) {
    emit_bytes(c, OP_SET_LOCAL, (uint8_t)idx);
    return;
  }

  idx = capture_upvalue(c->cur_scope, name);
  if (idx >= 0) {
    emit_bytes(c, OP_SET_UPVALUE, (uint8_t)idx);
    return;
  }

  // find in globals
  uint8_t constant = make_constant(c, name);
  emit_bytes(c, OP_SET_GLOBAL, constant);
}

static void getvar(Compiler *c, Value name)
{
  int idx;

  idx = find_local(c->cur_scope, name, FIND_ALL);
  if (idx >= 0) {
    emit_bytes(c, OP_GET_LOCAL, (uint8_t)idx);
    return;
  }

  idx = capture_upvalue(c->cur_scope, name);
  if (idx >= 0) {
    emit_bytes(c, OP_GET_UPVALUE, (uint8_t)idx);
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

// Context stores the parsing info of context expression,
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
typedef struct {
  token_t id;
  int arity;
  Value first;
} Context;

static Context empty_context(token_t id)
{
  return (Context){
    .arity = 0,
    .id = id,
  };
}

static Context unary_context(token_t id, Value first)
{
  return (Context){
    .arity = 1,
    .id = id,
    .first = first,
  };
}

static void eval(Compiler *c, Context context)
{
  if (context.arity == 0) {
    return;
  }
  if (context.id == TK_IDENT) {
    getvar(c, context.first);
  } else if (context.id == TK_DOT) {
    emit_bytes(c, OP_GET_FIELD, make_constant(c, context.first));
  } else {
    emit_constant(c, context.first);
  }
}

// nud_func denotes operator with one operand
typedef Context (*nud_func)(Compiler *);
// led_func denotes operator with more than one operands
typedef Context (*led_func)(Compiler *, Context);

typedef struct {
  nud_func nud;
  led_func led;
  binding_power bp;
} Symbol;

static Symbol symbols[TK_MAX + 1];

static inline int cur_pos(Compiler *c) { return chunk_len(c->cur_chunk); }

#define nud_of(token) (symbols[token.type].nud)
#define led_of(token) (symbols[token.type].led)
#define bp_of(token) (symbols[token.type].bp)

static Context expression(Compiler *c, binding_power bp)
{
  Context left;
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

static Context literal(Compiler *c)
{
  Value v;
  Token tk = prev(c);
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
    v = make_string(c, token_lexem_start(&tk) + 1, token_lexem_len(&tk) - 2);
    break;
  }
  return unary_context(tk.type, v);
}

static Context variable(Compiler *c)
{
  Token tk = prev(c);
  Value name = make_string(c, token_lexem_start(&tk), token_lexem_len(&tk));
  return unary_context(TK_IDENT, name);
}

static Context this(Compiler *c)
{
  if (!c->in_class) {
    errorf(c, "Can't use 'this' outside of a class.");
    return empty_context(TK_ERR);
  }
  return variable(c);
}

static Context _super(Compiler *c)
{
  if (!c->in_class) {
    errorf(c, "Can't use 'super' outside of a class.");
    return empty_context(TK_ERR);
  }
  if (!c->has_super) {
    errorf(c, "Can't use 'super' in a class with no superclass.");
    return empty_context(TK_ERR);
  }

  consume(c, TK_DOT, "Expect '.' after 'super'.");
  consume(c, TK_IDENT, "Expect superclass method name.");

  Value name = variable(c).first;

  getvar(c, make_string(c, "this", 4));
  getvar(c, make_string(c, "super", 5));
  emit_bytes(c, OP_GET_SUPER, make_constant(c, name));

  return empty_context(TK_SUPER);
}

static Context negative(Compiler *c)
{
  Context right = expression(c, BP_UNARY);
  eval(c, right);
  emit_byte(c, OP_NEGATIVE);
  return empty_context(TK_MINUS);
}

static Context not(Compiler * c)
{
  Context right = expression(c, BP_UNARY);
  eval(c, right);
  emit_byte(c, OP_NOT);
  return empty_context(TK_BANG);
}

static Context group(Compiler *c)
{
  Context grouped = expression(c, BP_NONE);
  consume(c, TK_RIGHT_PAREN, "Expect ')' after group.");
  eval(c, grouped);
  return empty_context(TK_LEFT_PAREN);
}

static Context ret(Compiler *c)
{
  if (is_global(c->cur_scope)) {
    errorf(c, "Can't return from top-level code.");
    return empty_context(TK_ERR);
  }

  if (c->in_initializer) {
    if (!check(c, TK_SEMICOLON)) {
      errorf(c, "Can't return a value from an initializer.");
      return empty_context(TK_ERR);
    }
    getvar(c, make_string(c, "this", 4));
  } else {
    if (!check(c, TK_SEMICOLON)) {
      eval(c, expression(c, BP_NONE));
    } else {
      emit_constant(c, value_make_nil());
    }
  }

  emit_return(c);
  return empty_context(TK_RETURN);
}

static Context assignment(Compiler *c, Context left)
{
  if (left.id != TK_IDENT && left.id != TK_DOT) {
    errorf(c, "Invalid assignment target.");
    return empty_context(TK_ERR);
  }
  if (left.id == TK_IDENT
      && value_equal(left.first, make_string(c, "this", 4))) {
    errorf(c, "Invalid assignment target.");
    return empty_context(TK_ERR);
  }

  Token tk = prev(c);
  Context right = expression(c, bp_of(tk) - 1);
  eval(c, right);
  if (left.id == TK_IDENT) {
    setvar(c, left.first);
  } else {
    emit_bytes(c, OP_SET_FIELD, make_constant(c, left.first));
  }
  return empty_context(tk.type);
}

static int parameters(Compiler *c)
{
  int arity = 0;
  while (match(c, TK_IDENT)) {
    if (arity >= 255) {
      errorf(c, "Can't have more than 255 parameters.");
      return arity;
    }
    defvar(c, variable(c).first);
    arity++;
    if (!match(c, TK_COMMA)) {
      break;
    }
  }
  consume(c, TK_RIGHT_PAREN, "Expect ')' after parameters.");
  return arity;
}

static int arguments(Compiler *c)
{
  int arity = 0;
  if (!check(c, TK_RIGHT_PAREN)) {
    do {
      eval(c, expression(c, BP_NONE));
      arity++;
      if (arity > UINT8_MAX) {
        errorf(c, "Can't have more than 255 arguments.");
        return arity;
      }
    } while (match(c, TK_COMMA));
  }
  consume(c, TK_RIGHT_PAREN, "Expect ')' after arguments.");
  return arity;
}

static Context dot(Compiler *c, Context left)
{
  consume(c, TK_IDENT, "Expect property name after '.'.");
  Value field = variable(c).first;
  eval(c, left);

  if (match(c, TK_LEFT_PAREN)) {
    int arity = arguments(c);
    emit_byte(c, OP_INVOKE);
    emit_bytes(c, arity, make_constant(c, field));
    return empty_context(TK_DOT);
  }

  return unary_context(TK_DOT, field);
}

static op_code infix_opcode(Token token)
{
  switch (token.type) {
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

static Context infix(Compiler *c, Context left)
{
  eval(c, left);
  Token tk = prev(c);
  Context right = expression(c, bp_of(tk));
  eval(c, right);
  emit_byte(c, infix_opcode(tk));
  return empty_context(tk.type);
}

static Context infix_and(Compiler *c, Context left)
{
  eval(c, left);
  int jmp_pos = emit_jmp(c, OP_JMP_ON_FALSE);
  emit_byte(c, OP_POP);
  eval(c, expression(c, BP_AND));
  patch_jmp(c, jmp_pos, cur_pos(c));
  return empty_context(TK_AND);
}

static Context infix_or(Compiler *c, Context left)
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

static Context call(Compiler *c, Context left)
{
  eval(c, left);
  int arity = arguments(c);
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

static void block_stmt(Compiler *);
static void if_stmt(Compiler *);
static void while_stmt(Compiler *);
static void for_stmt(Compiler *);
static void expr_stmt(Compiler *);
static void print_stmt(Compiler *);
static void statement(Compiler *);
static void var_declaration(Compiler *);
static void fun_declaration(Compiler *);
static void declaration(Compiler *);

static void block_stmt(Compiler *c)
{
  scope_in(c->cur_scope);

  while (!check(c, TK_RIGHT_BRACE) && !check(c, TK_EOF)) {
    declaration(c);
    if (c->panic) {
      return;
    }
  }
  consume(c, TK_RIGHT_BRACE, "Expect '}' after block.");

  scope_out(c->cur_scope, c);
}

static void if_stmt(Compiler *c)
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

static void while_stmt(Compiler *c)
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

static void for_stmt(Compiler *c)
{
  scope_in(c->cur_scope);

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

  scope_out(c->cur_scope, c);
}

static void expr_stmt(Compiler *c)
{
  eval(c, expression(c, BP_NONE));
  consume(c, TK_SEMICOLON, "Expect ';' after expression.");
  emit_byte(c, OP_POP);
}

static void print_stmt(Compiler *c)
{
  eval(c, expression(c, BP_NONE));
  consume(c, TK_SEMICOLON, "Expect ';' after print declaration.");
  emit_byte(c, OP_PRINT);
}

static void statement(Compiler *c)
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

static void var_declaration(Compiler *c)
{
  consume(c, TK_IDENT, "Expect variable name.");
  Context ctx = variable(c);
  Value name = ctx.first;

  if (find_local(c->cur_scope, name, FIND_CUR) != -1) {
    errorf(c, "Already a variable with this name in this scope.");
    return;
  }

  if (match(c, TK_EQUAL)) {
    Context initializer = expression(c, BP_NONE);

    bool in_local = !is_global(c->cur_scope);
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

static void function(Compiler *c, Value fname, bool is_method)
{
  consume(c, TK_LEFT_PAREN, "Expect '(' after function name.");

  Scope scope;
  scope_init(&scope);
  frame_enter(c, &scope);

  if (is_method) {
    defvar(c, make_string(c, "this", 4));
  } else {
    defvar(c, fname);
  }

  int arity = parameters(c);

  Value fun = value_make_fun(arity, as_string(fname));

  consume(c, TK_LEFT_BRACE, "Expect '{' before function body.");

  // switch compiling chunk
  Chunk *enclosing = c->cur_chunk;
  ObjectFunction *funobj = (ObjectFunction *)as_object(fun);
  c->cur_chunk = &funobj->chunk;

  block_stmt(c);

  if (c->in_initializer) {
    getvar(c, make_string(c, "this", 4));
  } else {
    emit_constant(c, value_make_nil());
  }
  emit_byte(c, OP_RETURN);

#ifdef DEBUG
  debug_chunk(c->cur_chunk, c->constants, as_string(fname)->str);
#endif

  funobj->upvalue_size = scope.upvalue_size;
  frame_out(c);

  // back to previous compiling chunk
  c->cur_chunk = enclosing;
  emit_bytes(c, OP_CLOSURE, make_constant(c, fun));
  for (int i = 0; i < scope.upvalue_size; i++) {
    emit_byte(c, (uint8_t)scope.upvalues[i].idx);
    emit_byte(c, scope.upvalues[i].from_local ? 1 : 0);
  }

  if (is_method) {
    emit_bytes(c, OP_METHOD, make_constant(c, fname));
  } else {
    defvar(c, fname);
  }
}

static void fun_declaration(Compiler *c)
{
  consume(c, TK_IDENT, "Expect function name.");
  Value fname = variable(c).first;
  if (find_local(c->cur_scope, fname, FIND_CUR) != -1) {
    errorf(c, "Already a variable with this name in this scope.");
    return;
  }

  bool old_in_initializer = c->in_initializer;
  c->in_initializer = false;

  function(c, fname, false /* is_method */);

  c->in_initializer = old_in_initializer;
}

static void method(Compiler *c)
{
  consume(c, TK_IDENT, "Expect method name.");
  Value fname = variable(c).first;

  bool old_in_initializer = c->in_initializer;
  if (value_equal(fname, make_string(c, "init", 4))) {
    c->in_initializer = true;
  }

  function(c, fname, true /* is_method */);

  c->in_initializer = old_in_initializer;
}

static void class_declaration(Compiler *c)
{
  bool old_in_class = c->in_class;
  bool old_has_super = c->has_super;
  c->in_class = true;

  consume(c, TK_IDENT, "Expect class name.");
  Value name = variable(c).first;

  emit_bytes(c, OP_CLASS, make_constant(c, name));
  defvar(c, name);

  scope_in(c->cur_scope);

  if (match(c, TK_LESS)) {

    c->has_super = true;

    consume(c, TK_IDENT, "Expect superclass name.");
    Context ctx = variable(c);
    if (value_equal(ctx.first, name)) {
      errorf(c, "A class can't inherit from itself.");
      return;
    }
    eval(c, ctx);

    defvar(c, make_string(c, "super", 5));
    getvar(c, name);
    emit_byte(c, OP_DERIVE);

  } else {
    c->has_super = false;
    getvar(c, name);
  }

  consume(c, TK_LEFT_BRACE, "Expect '{' before class body.");
  while (!check(c, TK_EOF) && !check(c, TK_RIGHT_BRACE)) {
    method(c);
  }
  consume(c, TK_RIGHT_BRACE, "Expect '}' after class body.");

  scope_out(c->cur_scope, c);

  c->in_class = old_in_class;
  c->has_super = old_has_super;
}

static void declaration(Compiler *c)
{
  if (match(c, TK_VAR)) {
    var_declaration(c);
  } else if (match(c, TK_FUN)) {
    fun_declaration(c);
  } else if (match(c, TK_CLASS)) {
    class_declaration(c);
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
  led_symbol(TK_DOT, BP_CALL, dot);
  just_symbol(TK_SEMICOLON);

  // Keywords
  just_symbol(TK_FOR);
  just_symbol(TK_PRINT);

  nud_symbol(TK_RETURN, ret);

  just_symbol(TK_CLASS);

  nud_symbol(TK_THIS, this);

  just_symbol(TK_ELSE);
  just_symbol(TK_IF);
  just_symbol(TK_VAR);
  just_symbol(TK_WHILE);
  just_symbol(TK_FUN);

  nud_symbol(TK_SUPER, _super);

  // eof
  just_symbol(TK_EOF);

  done = true;
}

static void compiler_init(Compiler *c, char *src)
{
  setup();

  c->panic = 0;
  c->error = 0;

  lex_init(&c->lexer, src, strlen(src));

  map_init(&c->interned_strings);
  map_init(&c->mconstants);

  // initial forward
  forward(c);
}

static int compile_chunk(char *src, Chunk *chunk, ValueArray *constants)
{
  Compiler c;
  compiler_init(&c, src);
  c.cur_chunk = chunk;
  c.constants = constants;
  make_constant(&c, make_string(&c, "init", 4));

  Scope root;
  scope_init(&root);
  scope_add(&root, make_string(&c, "script", 6));
  c.cur_scope = &root;

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

int compile(char *src, ObjectFunction *fun, ValueArray *constants)
{
#ifdef DEBUG
  time_t start = clock();
#endif

  int err = compile_chunk(src, &fun->chunk, constants);

#ifdef DEBUG
  fprintf(stderr, "compile time: %ds\n", (clock() - start) / CLOCKS_PER_SEC);
#endif

#ifdef DEBUG
  debug_chunk(&fun->chunk, constants, fun->name->str);
#endif

  return err;
}
