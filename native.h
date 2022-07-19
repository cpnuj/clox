#include "object.h"
#include "value.h"

typedef struct value (*native_fn)(int argc, struct value *argv);

struct obj_native {
  struct object base;
  int arity;
  native_fn method;
};

#define value_as_native(value)                                                 \
  (object_as(value_as_obj(value), struct obj_native))

#define is_native(value)                                                       \
  (is_object(value) && object_is(value_as_obj(value), OBJ_NATIVE))

struct value value_make_native(int, native_fn);

struct value native_clock(int, struct value *);
