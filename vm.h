#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

struct frame {
  int pc;
  int arity;
  struct chunk *chunk;
  struct value *bp; // base pointer of this frame
};

#define STACK_MAX 256
#define FRAME_MAX 256

struct vm {
  int done;
  int error;
  char errmsg[128];

  struct chunk chunk;
  struct map globals;
  struct value_list constants;

  struct value stack[STACK_MAX];
  struct value *sp; // Stack pointer

  struct frame frames[FRAME_MAX];
  int cur_frame;
};

void vm_init(struct vm *vm);
void vm_run(struct vm *vm);
void vm_push(struct vm *vm, struct value v);
struct value vm_pop(struct vm *vm);
struct value vm_top(struct vm *vm);

#endif
