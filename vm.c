#include <assert.h>

#include "debug.h"
#include "vm.h"

void vmerror (VM *vm, char *errmsg);

uint8_t fetchCode (VM *vm);
void runInstruction (VM *vm, uint8_t i);

void opConstant (VM *vm);
void opNegative (VM *vm);
void opBinaryNumber (VM *vm, uint8_t op);
void opBinaryBool (VM *vm, uint8_t op);

void initVM (VM *vm)
{
  vm->pc = 0;
  vm->sp = vm->stack - 1;
  vm->error = 0;
  initChunk (&vm->chunk);
}

void VMpush (VM *vm, Value v)
{
  if (vm->sp == vm->stack + STACK_MAX - 1)
  {
    // TODO: Add error handling
    return;
  }
  vm->sp++;
  *vm->sp = v;
}

Value VMpop (VM *vm)
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

void VMrun (VM *vm)
{
  while (1)
  {
    if (vm->pc >= vm->chunk.len)
    {
      vmerror (vm, "VM error: pc out of bound");
    }
    if (vm->error)
    {
      fprintf (stderr, "%s\n", vm->errmsg);
      return;
    }
    uint8_t instruction = fetchCode (vm);
    if (instruction == OP_RETURN)
    {
      return;
    }
    runInstruction (vm, instruction);
  }
}

void vmerror (VM *vm, char *errmsg)
{
  vm->error = 1;
  vm->errmsg = errmsg;
}

uint8_t fetchCode (VM *vm)
{
  if (vm->pc >= vm->chunk.len)
  {
    panic ("VM error: fetchCode overflow");
  }
  uint8_t code = vm->chunk.code[vm->pc];
  vm->pc++;
  return code;
}

void runInstruction (VM *vm, uint8_t i)
{
  switch (i)
  {
    case OP_CONSTANT:
      return opConstant (vm);
    case OP_NEGATIVE:
      return opNegative (vm);

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
      return opBinaryNumber (vm, i);

    case OP_AND:
    case OP_OR:
      return opBinaryBool (vm, i);
  }
  // TODO: panic on unknown code
  vmerror (vm, "unknown code");
}

void opConstant (VM *vm)
{
  uint8_t off = fetchCode (vm);
  VMpush (vm, vm->chunk.constants.value[off]);
}

void opNegative (VM *vm)
{
  Value value = VMpop (vm);
  if (value.Type != VT_NUM)
  {
    vmerror (vm, "type error: need number type");
    return;
  }
  double number = VALUE_AS_NUMBER (value);
  VMpush (vm, NewNumValue (-number));
}

void opBinaryNumber (VM *vm, uint8_t op)
{
  Value v2 = VMpop (vm);
  Value v1 = VMpop (vm);
  if (v1.Type != VT_NUM || v2.Type != VT_NUM)
  {
    vmerror (vm, "type error: need number type");
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
      goto pushNumber;
    case OP_MINUS:
      n = n1 - n2;
      goto pushNumber;
    case OP_MUL:
      n = n1 * n2;
      goto pushNumber;
    case OP_DIV:
      n = n1 / n2;
      goto pushNumber;

    // logic op
    case OP_BANG_EQUAL:
      comp = n1 != n2;
      goto pushBoolean;
    case OP_EQUAL_EQUAL:
      comp = n1 == n2;
      goto pushBoolean;
    case OP_GREATER:
      comp = n1 > n2;
      goto pushBoolean;
    case OP_GREATER_EQUAL:
      comp = n1 >= n2;
      goto pushBoolean;
    case OP_LESS:
      comp = n1 < n2;
      goto pushBoolean;
    case OP_LESS_EQUAL:
      comp = n1 <= n2;
      goto pushBoolean;
  }

pushNumber:
  VMpush (vm, NewNumValue (n));
  return;

pushBoolean:
  VMpush (vm, NewBoolValue (comp));
  return;
}

void opBinaryBool (VM *vm, uint8_t op)
{
  Value v2 = VMpop (vm);
  Value v1 = VMpop (vm);
  if (v1.Type != VT_BOOL || v2.Type != VT_BOOL)
  {
    vmerror (vm, "type error: need bool type");
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

  VMpush (vm, NewBoolValue (b));
}
