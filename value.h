#ifndef clox_value_h
#define clox_value_h

#include <stdbool.h>

typedef enum
{
  VT_NIL,
  VT_NUM,
  VT_BOOL,
} ValueType;

typedef struct
{
  ValueType Type;
  union
  {
    bool boolean;
    double number;
  } as;
} Value;

#define VALUE_AS_BOOL(value) (value.as.boolean)
#define VALUE_AS_NUMBER(value) (value.as.number)

Value value_make_bool (bool boolean);
Value value_make_number (double number);

typedef struct
{
  int len;
  int cap;
  Value *value;
} ValueArray;

void value_array_init (ValueArray *va);
void value_array_write (ValueArray *va, Value v);
void value_array_free (ValueArray *va);

void value_print (Value v);

#endif
