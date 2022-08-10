#ifndef clox_object_h
#define clox_object_h

#include <stdbool.h>
#include <stdint.h>

#include "chunk.h"
#include "map.h"
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
  Value closed;
  Value *location;
  struct ObjectUpvalue *next;
} ObjectUpValue;

ObjectUpValue *upvalue_new(Value *);
void upvalue_close(ObjectUpValue *);

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

Value native_clock(int, Value *);

typedef struct ObjectClass {
  Object base;
  ObjectString *name;
  Map methods;
} ObjectClass;

ObjectClass *class_new(ObjectString *);

typedef struct {
  Object base;
  ObjectClass *klass;
  Map fields;
} ObjectInstance;

ObjectInstance *instance_new(ObjectClass *);

typedef struct {
  Object base;
  ObjectClosure *method;
  ObjectInstance *receiver;
} ObjectBoundMethod;

ObjectBoundMethod *bound_method_new(ObjectClosure *, ObjectInstance *);

#define is_string(value)                                                       \
  (is_object(value) && object_is(as_object(value), OBJ_STRING))

#define is_fun(value)                                                          \
  (is_object(value) && object_is(as_object(value), OBJ_FUNCTION))

#define is_closure(value)                                                      \
  (is_object(value) && object_is(as_object(value), OBJ_CLOSURE))

#define is_native(value)                                                       \
  (is_object(value) && object_is(as_object(value), OBJ_NATIVE))

#define is_class(value)                                                        \
  (is_object(value) && object_is(as_object(value), OBJ_CLASS))

#define is_instance(value)                                                     \
  (is_object(value) && object_is(as_object(value), OBJ_INSTANCE))

#define is_bound_method(value)                                                 \
  (is_object(value) && object_is(as_object(value), OBJ_BOUND_METHOD))

// Macros cast value to specific object
#define as_string(value) (object_as(as_object(value), ObjectString))

#define as_function(value) (object_as(as_object(value), ObjectFunction))

#define as_closure(value) (object_as(as_object(value), ObjectClosure))

#define as_native(value) (object_as(as_object(value), ObjectNative))

#define as_class(value) (object_as(as_object(value), ObjectClass))

#define as_instance(value) (object_as(as_object(value), ObjectInstance))

#define as_bound_method(value) (object_as(as_object(value), ObjectBoundMethod))

Value value_make_ident(char *, int);
Value value_make_string(char *, int);
Value value_make_fun(int, ObjectString *);
Value value_make_closure(ObjectFunction *);
Value value_make_native(int, native_fn);

#endif
