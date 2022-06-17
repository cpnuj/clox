#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "debug.h"
#include "map.h"
#include "vm.h"

void vm_error(struct vm *vm, char *errmsg);
void vm_errorf(struct vm *vm, char *format, ...);

uint8_t fetch_code(struct vm *vm);
void run_instruction(struct vm *vm, uint8_t i);

void op_constant(struct vm *vm);
void op_negative(struct vm *vm);
void op_binary(struct vm *vm, uint8_t op);
void op_global(struct vm *vm);
void op_set(struct vm *vm);
void op_print(struct vm *vm);

static struct value resolve(struct vm *vm, struct value v);
static struct map *globals(struct vm *vm);

void vm_init(struct vm *vm)
{
  vm->pc = 0;
  vm->sp = vm->stack - 1;
  vm->error = 0;
  chunk_init(&vm->chunk);
}

void vm_push(struct vm *vm, struct value v)
{
  if (vm->sp == vm->stack + STACK_MAX - 1) {
    // TODO: Add error handling
    return;
  }
  vm->sp++;
  *vm->sp = v;
}

struct value vm_pop(struct vm *vm)
{
  if (vm->sp < vm->stack) {
    // TODO: Add error handling
    assert(0);
  }
  struct value v = *vm->sp;
  vm->sp--;
  return v;
}

void vm_run(struct vm *vm)
{
  while (1) {
    if (vm->pc >= vm->chunk.len) {
      vm_error(vm, "struct vm error: pc out of bound");
    }
    if (vm->error) {
      while (fetch_code(vm) != OP_RETURN)
        ;
      return;
    }
    uint8_t instruction = fetch_code(vm);
    if (instruction == OP_RETURN) {
      return;
    }
    run_instruction(vm, instruction);
  }
}

void vm_error(struct vm *vm, char *errmsg)
{
  vm->error = 1;
  sprintf(vm->errmsg, "%s", errmsg);
}

void vm_errorf(struct vm *vm, char *format, ...)
{
  vm->error = 1;
  va_list ap;
  va_start(ap, format);
  vsprintf(vm->errmsg, format, ap);
}

uint8_t fetch_code(struct vm *vm)
{
  if (vm->pc >= vm->chunk.len) {
    panic("struct vm error: fetch_code overflow");
  }
  uint8_t code = vm->chunk.code[vm->pc];
  vm->pc++;
  return code;
}

void run_instruction(struct vm *vm, uint8_t i)
{
  switch (i) {
  case OP_CONSTANT:
    return op_constant(vm);
  case OP_NEGATIVE:
    return op_negative(vm);

  case OP_ADD:
  case OP_MINUS:
  case OP_MUL:
  case OP_DIV:
  case OP_BANG:
  case OP_BANG_EQUAL:
  case OP_EQUAL:
  case OP_EQUAL_EQUAL:
  case OP_GREATER:
  case OP_GREATER_EQUAL:
  case OP_LESS:
  case OP_LESS_EQUAL:
  case OP_AND:
  case OP_OR:
    return op_binary(vm, i);

  case OP_GLOBAL:
    return op_global(vm);

  case OP_SET:
    return op_set(vm);

  case OP_PRINT:
    return op_print(vm);
  }
  // TODO: panic on unknown code
  vm_error(vm, "unknown code");
}

void op_constant(struct vm *vm)
{
  uint8_t off = fetch_code(vm);
  vm_push(vm, vm->chunk.constants.value[off]);
}

void op_negative(struct vm *vm)
{
  struct value value = vm_pop(vm);
  if (value.type != VT_NUM) {
    vm_error(vm, "type error: need number type");
    return;
  }
  double number = value_as_number(value);
  vm_push(vm, value_make_number(-number));
}

struct value concatenate(struct value v1, struct value v2)
{
  struct obj_string *s1, *s2;
  s1 = value_as_string(v1);
  s2 = value_as_string(v2);
  return value_make_object(string_concat(s1, s2));
}

// resolve resolves the given value to an actual value. For ident value,
// return the coresponding value of the ident. Else, return the value itself.
static struct value resolve(struct vm *vm, struct value v)
{
  if (is_ident(v)) {
    if (!map_get(globals(vm), v, &v)) {
      vm_errorf(vm, "unknown variable %s", value_as_string(v)->str);
      return value_make_nil();
    }
  }
  return v;
}

void op_binary(struct vm *vm, uint8_t op)
{
  struct value v2 = vm_pop(vm);
  struct value v1 = vm_pop(vm);

  v1 = resolve(vm, v1);
  v2 = resolve(vm, v2);
  if (vm->error) {
    return;
  }

  struct value v;
  int error = 0;

#define op_calculation(op)                                                     \
  if (is_number(v1) && is_number(v2))                                          \
    v = value_make_number(value_as_number(v1) op value_as_number(v2));         \
  else                                                                         \
    error = 1;

#define op_comparison(op)                                                      \
  if (is_number(v1) && is_number(v2))                                          \
    v = value_make_bool(value_as_number(v1) op value_as_number(v2));           \
  else                                                                         \
    error = 1;

#define op_logic(op)                                                           \
  if (is_bool(v1) && is_bool(v2))                                              \
    v = value_make_bool(value_as_bool(v1) op value_as_bool(v2));               \
  else                                                                         \
    error = 1;

  switch (op) {
  // calculation
  case OP_ADD:
    if (is_number(v1) && is_number(v2))
      v = value_make_number(value_as_number(v1) + value_as_number(v2));
    else if (is_string(v1) && is_string(v2))
      v = concatenate(v1, v2);
    else
      error = 1;
    break;

  case OP_MINUS:
    op_calculation(-);
    break;

  case OP_MUL:
    op_calculation(*);
    break;

  case OP_DIV:
    op_calculation(/);
    break;

  // comparision
  case OP_BANG_EQUAL:
    op_comparison(!=);
    break;

  case OP_EQUAL_EQUAL:
    op_comparison(==);
    break;

  case OP_GREATER:
    op_comparison(>);
    break;

  case OP_GREATER_EQUAL:
    op_comparison(>=);
    break;

  case OP_LESS:
    op_comparison(<);
    break;

  case OP_LESS_EQUAL:
    op_comparison(<=);
    break;

  // logic
  case OP_AND:
    op_logic(&&);
    break;

  case OP_OR:
    op_logic(||);
    break;
  }

  if (error)
    // TODO: make error msg clear
    vm_error(vm, "type error");
  else
    vm_push(vm, v);

#undef BINARY_OP_CAL
#undef BINARY_OP_COMP
#undef BINARY_OP_LOGIC
}

static struct map *globals(struct vm *vm) { return &vm->chunk.globals; }

void op_global(struct vm *vm)
{
  struct value value = vm_pop(vm);
  struct value name = vm_pop(vm);
  if (!is_ident(name)) {
    panic("programming error: OP_GLOBAL operates on a non-ident name");
  }
  map_put(globals(vm), name, value);
}

void op_set(struct vm *vm)
{
  struct value value = vm_pop(vm);
  struct value name = vm_pop(vm);
  if (!is_ident(name)) {
    vm_error(vm, "cannot assign to a non-ident value");
  }
  // use resolve to check whether it is a defined variable
  resolve(vm, name);
  if (vm->error) {
    return;
  }
  map_put(globals(vm), name, value);
}

void op_print(struct vm *vm)
{
  struct value value = vm_pop(vm);
  value = resolve(vm, value);
  if (vm->error) {
    return;
  }
  value_print(value);
  printf("\n");
}
