#ifndef clox_value_h
#define clox_value_h

#include <stdbool.h>
#include <stdint.h>

#include "object.h"

typedef enum {
  VT_NIL,
  VT_NUM,
  VT_BOOL,
  VT_OBJ,
  VT_IDENT,
} value_t;

typedef struct {
  value_t type;
  union {
    bool boolean;
    double number;
    Object *obj;
  } as;
} Value;

#define value_as_bool(value) (value.as.boolean)
#define value_as_number(value) (value.as.number)
#define value_as_obj(value) (value.as.obj)

#define is_nil(value) (value.type == VT_NIL)
#define is_number(value) (value.type == VT_NUM)
#define is_bool(value) (value.type == VT_BOOL)
#define is_object(value) (value.type == VT_OBJ)
#define is_ident(value) (value.type == VT_IDENT)

#define is_string(value)                                                       \
  (is_object(value) && object_is(value_as_obj(value), OBJ_STRING))

#define is_fun(value)                                                          \
  (is_object(value) && object_is(value_as_obj(value), OBJ_FUNCTION))

#define is_closure(value)                                                      \
  (is_object(value) && object_is(value_as_obj(value), OBJ_CLOSURE))

// Macros cast value to specific object
#define value_as_string(value) (object_as(value_as_obj(value), ObjectString))

#define value_as_fun(value) (object_as(value_as_obj(value), ObjectFunction))

#define value_as_closure(value) (object_as(value_as_obj(value), ObjectClosure))

Value value_make_nil(void);
Value value_make_bool(bool);
Value value_make_number(double);
Value value_make_object(Object *);
Value value_make_ident(char *, int);
Value value_make_string(char *, int);
Value value_make_fun(int, ObjectString *);
Value value_make_closure(ObjectFunction *);

uint32_t value_hash(Value);
bool value_is_false(Value);
bool value_equal(Value, Value);
void value_print(Value v);

typedef struct {
  int len;
  int cap;
  Value *value;
} ValueArray;

void value_array_init(ValueArray *va);
void value_array_write(ValueArray *va, Value v);
void value_array_free(ValueArray *va);
Value value_array_get(ValueArray *vlist, int idx);

#endif
