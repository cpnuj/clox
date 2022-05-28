#include <assert.h>

#include "debug.h"
#include "vm.h"

void vm_error (VM *vm, char *errmsg);

uint8_t fetch_code (VM *vm);
void run_instruction (VM *vm, uint8_t i);

void op_constant (VM *vm);
void op_negative (VM *vm);
void op_binary (VM *vm, uint8_t op);

void vm_init (VM *vm)
{
  vm->pc = 0;
  vm->sp = vm->stack - 1;
  vm->error = 0;
  chunk_init (&vm->chunk);
}

void vm_push (VM *vm, Value v)
{
  if (vm->sp == vm->stack + STACK_MAX - 1)
  {
    // TODO: Add error handling
    return;
  }
  vm->sp++;
  *vm->sp = v;
}

Value vm_pop (VM *vm)
{
  if (vm->sp < vm->stack)
  {
    // TODO: Add error handling
    assert (0);
  }
  Value v = *vm->sp;
  vm->sp--;
  return v;
}

void vm_run (VM *vm)
{
  while (1)
  {
    if (vm->pc >= vm->chunk.len)
    {
      vm_error (vm, "VM error: pc out of bound");
    }
    if (vm->error)
    {
      fprintf (stderr, "%s\n", vm->errmsg);
      return;
    }
    uint8_t instruction = fetch_code (vm);
    if (instruction == OP_RETURN)
    {
      return;
    }
    run_instruction (vm, instruction);
  }
}

void vm_error (VM *vm, char *errmsg)
{
  vm->error = 1;
  vm->errmsg = errmsg;
}

uint8_t fetch_code (VM *vm)
{
  if (vm->pc >= vm->chunk.len)
  {
    panic ("VM error: fetch_code overflow");
  }
  uint8_t code = vm->chunk.code[vm->pc];
  vm->pc++;
  return code;
}

void run_instruction (VM *vm, uint8_t i)
{
  switch (i)
  {
    case OP_CONSTANT:
      return op_constant (vm);
    case OP_NEGATIVE:
      return op_negative (vm);

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
      return op_binary (vm, i);
  }
  // TODO: panic on unknown code
  vm_error (vm, "unknown code");
}

void op_constant (VM *vm)
{
  uint8_t off = fetch_code (vm);
  vm_push (vm, vm->chunk.constants.value[off]);
}

void op_negative (VM *vm)
{
  Value value = vm_pop (vm);
  if (value.Type != VT_NUM)
  {
    vm_error (vm, "type error: need number type");
    return;
  }
  double number = value_as_number (value);
  vm_push (vm, value_make_number (-number));
}

Value concatenate (Value v1, Value v2)
{
  struct obj_string *s1, *s2;
  s1 = value_as_string (v1);
  s2 = value_as_string (v2);
  return value_make_object (string_concat (s1, s2));
}

void op_binary (VM *vm, uint8_t op)
{
  Value v2 = vm_pop (vm);
  Value v1 = vm_pop (vm);

  Value v;
  int error = 0;

#define op_calculation(op)                                                    \
  if (is_number (v1) && is_number (v2))                                       \
    v = value_make_number (value_as_number (v1) op value_as_number (v2));     \
  else                                                                        \
    error = 1;

#define op_comparison(op)                                                     \
  if (is_number (v1) && is_number (v2))                                       \
    v = value_make_bool (value_as_number (v1) op value_as_number (v2));       \
  else                                                                        \
    error = 1;

#define op_logic(op)                                                          \
  if (is_bool (v1) && is_bool (v2))                                           \
    v = value_make_bool (value_as_bool (v1) op value_as_bool (v2));           \
  else                                                                        \
    error = 1;

  switch (op)
  {
    // calculation
    case OP_ADD:
      if (is_number (v1) && is_number (v2))
        v = value_make_number (value_as_number (v1) + value_as_number (v2));
      else if (is_string (v1) && is_string (v2))
        v = concatenate (v1, v2);
      else
        error = 1;
      break;

    case OP_MINUS:
      op_calculation (-);
      break;

    case OP_MUL:
      op_calculation (*);
      break;

    case OP_DIV:
      op_calculation (/);
      break;

    // comparision
    case OP_BANG_EQUAL:
      op_comparison (!=);
      break;

    case OP_EQUAL_EQUAL:
      op_comparison (==);
      break;

    case OP_GREATER:
      op_comparison (>);
      break;

    case OP_GREATER_EQUAL:
      op_comparison (>=);
      break;

    case OP_LESS:
      op_comparison (<);
      break;

    case OP_LESS_EQUAL:
      op_comparison (<=);
      break;

    // logic
    case OP_AND:
      op_logic (&&);
      break;

    case OP_OR:
      op_logic (||);
      break;
  }

  if (error)
    // TODO: make error msg clear
    vm_error (vm, "type error");
  else
    vm_push (vm, v);

#undef BINARY_OP_CAL
#undef BINARY_OP_COMP
#undef BINARY_OP_LOGIC
}
