#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "value.h"

#define STACK_MAX 256

typedef struct {
    int pc;     // Program counter
    Value *sp;  // Stack pointer

    Chunk chunk;
    Value stack[STACK_MAX];
} VM;

void initVM(VM *vm);
void VMrun(VM *vm);
void VMpush(VM *vm, Value v);
Value VMpop(VM *vm); 

#endif

