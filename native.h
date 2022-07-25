#include "object.h"
#include "value.h"

typedef Value (*native_fn)(int argc, Value *argv);

typedef struct {
  Object base;
  int arity;
  native_fn method;
} ObjectNative;

#define value_as_native(value) (object_as(value_as_obj(value), ObjectNative))

#define is_native(value)                                                       \
  (is_object(value) && object_is(value_as_obj(value), OBJ_NATIVE))

Value value_make_native(int, native_fn);

Value native_clock(int, Value *);
