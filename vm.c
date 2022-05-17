#include <assert.h>

#include "vm.h"

uint8_t fetchCode(VM *vm);
void runInstruction(VM *vm, uint8_t i);

void opConstant(VM *vm);
void opNegative(VM *vm);
void opBinary(VM *vm, uint8_t op);

void initVM(VM *vm) {
    vm->pc = 0;
    vm->sp = vm->stack - 1;
    initChunk(&vm->chunk);
}

void VMpush(VM *vm, Value v) {
    if (vm->sp == vm->stack + STACK_MAX - 1) {
        // TODO: Add error handling
        return;
    }
    vm->sp++;
    *vm->sp = v;
}

Value VMpop(VM *vm) {
    if (vm->sp < vm->stack) {
        // TODO: Add error handling
        assert(0);
    }
    Value v = *vm->sp;
    vm->sp--;
    return v;
}

void VMrun(VM *vm) {
    while (vm->pc < vm->chunk.len) {
        uint8_t instruction = fetchCode(vm);
        if (instruction == OP_RETURN) {
            return;
        }
        runInstruction(vm, instruction);
    }
}

uint8_t fetchCode(VM *vm) {
    if (vm->pc >= vm->chunk.len) {
        // TODO: panic
    }
    uint8_t code = vm->chunk.code[vm->pc];
    vm->pc++;
    return code;
}

void runInstruction(VM *vm, uint8_t i) {
    switch (i) {
    case OP_CONSTANT:
        return opConstant(vm);
    case OP_NEGATIVE:
        return opNegative(vm);

    case OP_ADD:
    case OP_MINUS:
    case OP_MUL:
    case OP_DIV:
        return opBinary(vm, i);
    }
    // TODO: panic on unknown code
}

void opConstant(VM *vm) {
    uint8_t off = fetchCode(vm);
    VMpush(vm, vm->chunk.constants.value[off]);
}

void opNegative(VM *vm) {
    Value value = VMpop(vm);
    VMpush(vm, -value);
}

void opBinary(VM *vm, uint8_t op) {
    Value v2 = VMpop(vm);
    Value v1 = VMpop(vm);
    Value v;
    switch (op) {
    case OP_ADD:
        v = v1 + v2;
        break;
    case OP_MINUS:
        v = v1 - v2;
        break;
    case OP_MUL:
        v = v1 * v2;
        break;
    case OP_DIV:
        v = v1 / v2;
        break;
    }
    VMpush(vm, v);
}

