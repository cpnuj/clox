#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

struct vm {
  int pc;           // Program counter
  struct value *sp; // Stack pointer

  struct chunk chunk;
  struct value stack[STACK_MAX];

  int error;
  char errmsg[128];
};

void vm_init(struct vm *vm);
void vm_run(struct vm *vm);
void vm_push(struct vm *vm, struct value v);
struct value vm_pop(struct vm *vm);
struct value vm_top(struct vm *vm);

#endif
