#ifndef clox_value_h
#define clox_value_h

#include <stdbool.h>

typedef enum {
    VT_NIL,
    VT_NUM,
    VT_BOOL,
} ValueType;

typedef struct {
    ValueType Type;
    union {
        bool boolean;
        double number;
    } as;
} Value;

#define VALUE_AS_BOOL(value)   (value.as.boolean)
#define VALUE_AS_NUMBER(value) (value.as.number)

Value NewBoolValue(bool boolean);
Value NewNumValue(double number);

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
