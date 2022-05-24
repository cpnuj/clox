#include <assert.h>

#include "debug.h"
#include "vm.h"

void vm_error (VM *vm, char *errmsg);

uint8_t fetch_code (VM *vm);
void run_instruction (VM *vm, uint8_t i);

void op_constant (VM *vm);
void op_negative (VM *vm);
void op_binary_number (VM *vm, uint8_t op);
void op_binary_bool (VM *vm, uint8_t op);

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
      return op_binary_number (vm, i);

    case OP_AND:
    case OP_OR:
      return op_binary_bool (vm, i);
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
  double number = VALUE_AS_NUMBER (value);
  vm_push (vm, value_make_number (-number));
}

void op_binary_number (VM *vm, uint8_t op)
{
  Value v2 = vm_pop (vm);
  Value v1 = vm_pop (vm);
  if (v1.Type != VT_NUM || v2.Type != VT_NUM)
  {
    vm_error (vm, "type error: need number type");
    return;
  }

  double n1 = VALUE_AS_NUMBER (v1);
  double n2 = VALUE_AS_NUMBER (v2);

  double n;
  bool comp;
  switch (op)
  {
    // number op
    case OP_ADD:
      n = n1 + n2;
      goto push_number;
    case OP_MINUS:
      n = n1 - n2;
      goto push_number;
    case OP_MUL:
      n = n1 * n2;
      goto push_number;
    case OP_DIV:
      n = n1 / n2;
      goto push_number;

    // logic op
    case OP_BANG_EQUAL:
      comp = n1 != n2;
      goto push_boolean;
    case OP_EQUAL_EQUAL:
      comp = n1 == n2;
      goto push_boolean;
    case OP_GREATER:
      comp = n1 > n2;
      goto push_boolean;
    case OP_GREATER_EQUAL:
      comp = n1 >= n2;
      goto push_boolean;
    case OP_LESS:
      comp = n1 < n2;
      goto push_boolean;
    case OP_LESS_EQUAL:
      comp = n1 <= n2;
      goto push_boolean;
  }

push_number:
  vm_push (vm, value_make_number (n));
  return;

push_boolean:
  vm_push (vm, value_make_bool (comp));
  return;
}

void op_binary_bool (VM *vm, uint8_t op)
{
  Value v2 = vm_pop (vm);
  Value v1 = vm_pop (vm);
  if (v1.Type != VT_BOOL || v2.Type != VT_BOOL)
  {
    vm_error (vm, "type error: need bool type");
    return;
  }

  bool b;
  bool b1 = VALUE_AS_BOOL (v1);
  bool b2 = VALUE_AS_BOOL (v2);
  switch (op)
  {
    case OP_AND:
      b = b1 && b2;
      break;
    case OP_OR:
      b = b1 || b2;
      break;
  }

  vm_push (vm, value_make_bool (b));
}
