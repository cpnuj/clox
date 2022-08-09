#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "object.h"
#include "value.h"

typedef struct {
  int pc;
  Value *bp; // base pointer of this frame
  ObjectClosure *closure;
} CallFrame;

#define STACK_MAX 256
#define FRAME_MAX 256

typedef struct {
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

  CallFrame frames[FRAME_MAX];
  int cur_frame;

  // open_upvalues maintain upvalues still in stack
  ObjectUpValue *open_upvalues;

  // gc_threshold is the threshold for next gc.
  unsigned int gc_threshold;
} VM;

void vm_init(VM *vm);
void vm_run(VM *vm);
void vm_push(VM *vm, Value v);
Value vm_pop(VM *vm);
Value vm_top(VM *vm);

#endif
