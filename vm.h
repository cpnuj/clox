#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct
{
  int pc;    // Program counter
  Value *sp; // Stack pointer

  Chunk chunk;
  Value stack[STACK_MAX];

  int error;
  char *errmsg;
} VM;

void vm_init (VM *vm);
void vm_run (VM *vm);
void vm_push (VM *vm, Value v);
Value vm_pop (VM *vm);

#endif
