#include <stddef.h>
#include <stdio.h>

#include "memory.h"
#include "value.h"

void initValueArray(ValueArray* va) {
    va->len = 0;
    va->cap = 0;
    va->value = NULL;
}

void writeValueArray(ValueArray* va, Value v) {
    if (va->cap < va->len+1) {
        int oldSize = va->cap;
        va->cap = GROW_CAP(va->cap);
        va->value = GROW_ARRAY(Value, va->value, oldSize, va->cap);
    }
    va->value[va->len] = v;
    va->len++;
}

void freeValueArray(ValueArray* va) {
    FREE_ARRAY(Value, va->value, va->cap);
    initValueArray(va);
}

void printValue(Value v) {
    printf("%g", v);
}