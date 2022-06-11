#ifndef clox_value_h
#define clox_value_h

#include <stdbool.h>

#include "object.h"

typedef enum
{
  VT_NIL,
  VT_NUM,
  VT_BOOL,
  VT_OBJ,
} ValueType;

typedef struct
{
  ValueType Type;
  union
  {
    bool boolean;
    double number;
    OBJ *obj;
  } as;
} Value;

#define value_as_bool(value) (value.as.boolean)
#define value_as_number(value) (value.as.number)
#define value_as_obj(value) (value.as.obj)

#define is_nil(value) (value.Type == VT_NIL)
#define is_number(value) (value.Type == VT_NUM)
#define is_bool(value) (value.Type == VT_BOOL)
#define is_object(value) (value.Type == VT_OBJ)

#define is_string(value)                                                      \
  (is_object (value) && object_is (value_as_obj (value), OBJ_STRING))

// Macros cast value to specific object
#define value_as_string(value)                                                \
  (object_as (value_as_obj (value), struct obj_string))

Value value_make_nil (void);
Value value_make_bool (bool);
Value value_make_number (double);
Value value_make_object (OBJ *);

Value value_make_string (char *str, int len);

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
