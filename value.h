#ifndef clox_value_h
#define clox_value_h

typedef double Value;

typedef struct {
    int len;
    int cap;
    Value* value;
} ValueArray;

void initValueArray(ValueArray* va);
void writeValueArray(ValueArray* va, Value v);
void freeValueArray(ValueArray* va);

void printValue(Value v);

#endif