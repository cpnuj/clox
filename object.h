#ifndef clox_object_h
#define clox_object_h

#include <stdbool.h>
#include <stdint.h>

#include "chunk.h"
#include "value.h"

// ObjectString represents a string object in clox.
// If the raw string is embeded within this struct, pointer str points to filed
// raw, else str may point to other space specified by user.
typedef struct {
  Object base;
  int len;
  char *str;
  char raw[];
} ObjectString;

Object *string_copy(char *, int);
Object *string_take(char *, int);

Object *string_concat(ObjectString *, ObjectString *);
bool string_equal(Object *, Object *);

#define nohash 0

// ObjectFunction represents a function object in clox.
typedef struct {
  Object base;
  ObjectString *name;
  int arity;
  Chunk chunk;
  int upvalue_size;
} ObjectFunction;

Object *fun_new(int, ObjectString *);

typedef struct Value Value;

typedef struct ObjectUpvalue {
  Object base;
  bool closed;
  Value *location;
  struct ObjectUpvalue *next;
} ObjectUpValue;

ObjectUpValue *upvalue_new(Value *);
void upvalue_close(ObjectUpValue *, Value *);

// ObjectClosure is created as a running function object in runtime
typedef struct {
  Object base;
  ObjectFunction *proto;
  int upvalue_size;
  ObjectUpValue **upvalues;
} ObjectClosure;

ObjectClosure *closure_new(ObjectFunction *);

typedef Value (*native_fn)(int argc, Value *argv);

typedef struct {
  Object base;
  int arity;
  native_fn method;
} ObjectNative;

#define value_as_native(value) (object_as(value_as_obj(value), ObjectNative))

#define is_native(value)                                                       \
  (is_object(value) && object_is(value_as_obj(value), OBJ_NATIVE))

Value native_clock(int, Value *);

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

Value value_make_ident(char *, int);
Value value_make_string(char *, int);
Value value_make_fun(int, ObjectString *);
Value value_make_closure(ObjectFunction *);
Value value_make_native(int, native_fn);

#endif
