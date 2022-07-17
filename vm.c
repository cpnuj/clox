#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include "debug.h"
#include "map.h"
#include "vm.h"

void vm_error(struct vm *vm, char *errmsg);
void vm_errorf(struct vm *vm, char *format, ...);

uint8_t fetch_code(struct vm *vm);
struct value fetch_constant(struct vm *vm);
int fetch_int16(struct vm *vm);
void run_instruction(struct vm *vm, uint8_t i);

void op_constant(struct vm *vm);
void op_negative(struct vm *vm);
void op_not(struct vm *vm);
void op_binary(struct vm *vm, uint8_t op);
void op_global(struct vm *vm);
void op_set_global(struct vm *vm);
void op_get_global(struct vm *vm);
void op_set_local(struct vm *vm);
void op_get_local(struct vm *vm);
void op_jmp(struct vm *vm);
void op_jmp_back(struct vm *vm);
void op_jmp_on_false(struct vm *vm);
void op_print(struct vm *vm);

static struct map *globals(struct vm *vm);

static void vm_debug(struct vm *vm);

void vm_init(struct vm *vm)
{
  vm->pc = 0;
  vm->sp = vm->stack - 1;
  vm->error = 0;
  chunk_init(&vm->chunk);
  value_array_init(&vm->constants);
  map_init(&vm->globals);
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

struct value vm_top(struct vm *vm)
{
  if (vm->sp < vm->stack) {
    // TODO: Add error handling
    assert(0);
  }
  return *vm->sp;
}

void vm_run(struct vm *vm)
{
  while (1) {
    // vm_debug(vm);
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
  int printed = vsprintf(vm->errmsg, format, ap);
  int err_idx = vm->pc - 1;
  sprintf(vm->errmsg + printed, "\n[line %d] in script",
          vm->chunk.lines[err_idx]);
}

// fetch_code fetch and return the next code from vm chunk.
uint8_t fetch_code(struct vm *vm)
{
  if (vm->pc >= vm->chunk.len) {
    panic("struct vm error: fetch_code overflow");
  }
  uint8_t code = vm->chunk.code[vm->pc];
  vm->pc++;
  return code;
}

struct value fetch_constant(struct vm *vm)
{
  uint8_t off = fetch_code(vm);
  return vm->constants.value[off];
}

int fetch_int16(struct vm *vm)
{
  int h8 = fetch_code(vm); // high 8 bit
  int l8 = fetch_code(vm); // low  8 bit
  return (h8 << 8) | l8;
}

void run_instruction(struct vm *vm, uint8_t i)
{
  switch (i) {
  case OP_CONSTANT:
    return op_constant(vm);
  case OP_NEGATIVE:
    return op_negative(vm);
  case OP_NOT:
    return op_not(vm);

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
    return op_binary(vm, i);

  case OP_POP:
    vm_pop(vm);
    return;

  case OP_GLOBAL:
    return op_global(vm);

  case OP_SET_GLOBAL:
    return op_set_global(vm);
  case OP_GET_GLOBAL:
    return op_get_global(vm);

  case OP_SET_LOCAL:
    return op_set_local(vm);
  case OP_GET_LOCAL:
    return op_get_local(vm);

  case OP_JMP:
    return op_jmp(vm);
  case OP_JMP_BACK:
    return op_jmp_back(vm);
  case OP_JMP_ON_FALSE:
    return op_jmp_on_false(vm);

  case OP_PRINT:
    return op_print(vm);
  }
  // TODO: panic on unknown code
  vm_error(vm, "unknown code");
}

void op_constant(struct vm *vm) { vm_push(vm, fetch_constant(vm)); }

void op_negative(struct vm *vm)
{
  struct value value = vm_pop(vm);
  if (value.type != VT_NUM) {
    vm_errorf(vm, "Operand must be a number.");
    return;
  }
  double number = value_as_number(value);
  vm_push(vm, value_make_number(-number));
}

void op_not(struct vm *vm)
{
  struct value value = vm_pop(vm);
  if (value.type != VT_BOOL) {
    vm_error(vm, "type error: need boolean type");
    return;
  }
  bool boolean = value_as_bool(value);
  vm_push(vm, value_make_bool(!boolean));
}

struct value concatenate(struct value v1, struct value v2)
{
  struct obj_string *s1, *s2;
  s1 = value_as_string(v1);
  s2 = value_as_string(v2);
  return value_make_object(string_concat(s1, s2));
}

void op_binary(struct vm *vm, uint8_t op)
{
  struct value v2 = vm_pop(vm);
  struct value v1 = vm_pop(vm);

  struct value v;
  int error = 0;

#define op_calculation(op)                                                     \
  if (is_number(v1) && is_number(v2))                                          \
    v = value_make_number(value_as_number(v1) op value_as_number(v2));         \
  else                                                                         \
    vm_errorf(vm, "Operands must be numbers.")

#define op_comparison(op)                                                      \
  if (is_number(v1) && is_number(v2))                                          \
    v = value_make_bool(value_as_number(v1) op value_as_number(v2));           \
  else                                                                         \
    vm_errorf(vm, "Operands must be numbers.")

  switch (op) {
  // calculation
  case OP_ADD:
    if (is_number(v1) && is_number(v2))
      v = value_make_number(value_as_number(v1) + value_as_number(v2));
    else if (is_string(v1) && is_string(v2))
      v = concatenate(v1, v2);
    else
      vm_errorf(vm, "Operands must be two numbers or two strings.");
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

  // comparision '!=' '==' work on all value
  case OP_BANG_EQUAL:
    v = value_make_bool(!value_equal(v1, v2));
    break;

  case OP_EQUAL_EQUAL:
    v = value_make_bool(value_equal(v1, v2));
    break;

  // comparision on numbers
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

static struct map *globals(struct vm *vm) { return &vm->globals; }

void op_global(struct vm *vm)
{
  struct value name = fetch_constant(vm);
  struct value value = vm_pop(vm);
  if (!is_ident(name)) {
    panic("programming error: OP_GLOBAL operates on a non-ident name");
  }
  map_put(globals(vm), name, value);
}

void op_set_global(struct vm *vm)
{
  struct value name = fetch_constant(vm);
  struct value value = vm_pop(vm);

  if (!map_get(globals(vm), name, NULL)) {
    vm_errorf(vm, "Undefined variable '%s'.", value_as_string(name)->str);
    return;
  }

  map_put(globals(vm), name, value);
  vm_push(vm, value);
}

void op_get_global(struct vm *vm)
{
  struct value name = fetch_constant(vm);
  struct value value;

  if (!map_get(globals(vm), name, &value)) {
    vm_errorf(vm, "Undefined variable '%s'.", value_as_string(name)->str);
    return;
  }
  vm_push(vm, value);
}

void op_set_local(struct vm *vm)
{
  uint8_t off = fetch_code(vm);
  struct value *plocal = &vm->stack[off];
  struct value value = vm_pop(vm);
  *plocal = value;
  vm_push(vm, value);
}

void op_get_local(struct vm *vm)
{
  uint8_t off = fetch_code(vm);
  struct value *plocal = &vm->stack[off];
  vm_push(vm, *plocal);
}

void op_jmp(struct vm *vm) { vm->pc += fetch_int16(vm); }

void op_jmp_back(struct vm *vm) { vm->pc -= fetch_int16(vm); }

// op_jmp_on_false does not pop the value
void op_jmp_on_false(struct vm *vm)
{
  int offset = fetch_int16(vm);
  if (value_is_false(vm_top(vm))) {
    vm->pc += offset;
  }
}

void op_print(struct vm *vm)
{
  struct value value = vm_pop(vm);
  if (vm->error) {
    return;
  }
  value_print(value);
  printf("\n");
}

static void vm_debug(struct vm *vm)
{
  printf("======= DEBUG VM ======\n");
  printf("PC: %4d NEXT OP: ", vm->pc);
  // debug_instruction(&vm->chunk, vm->pc);
  printf("STACK\n");
  for (struct value *i = vm->stack; i <= vm->sp; i++) {
    value_print(*i);
    printf("\n");
  }
  printf("\n");
}