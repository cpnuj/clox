#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

struct frame {
  int pc;
  Value *bp; // base pointer of this frame
  ObjectClosure *closure;
};

#define STACK_MAX 256
#define FRAME_MAX 256

struct vm {
  int done;
  int error;
  char errmsg[128];

  Value vmain;
  ObjectFunction *main;
  ObjectClosure *main_closure;

  Map globals;
  ValueArray constants;

  Value stack[STACK_MAX];
  Value *sp; // Stack pointer

  struct frame frames[FRAME_MAX];
  int cur_frame;
};

void vm_init(struct vm *vm);
void vm_run(struct vm *vm);
void vm_push(struct vm *vm, Value v);
Value vm_pop(struct vm *vm);
Value vm_top(struct vm *vm);

#endif
