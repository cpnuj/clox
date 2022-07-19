#include <time.h>

#include "memory.h"
#include "native.h"

struct value value_make_native(int arity, native_fn method)
{
  struct value value;
  value.type = VT_OBJ;

  struct obj_native *obj
      = (struct obj_native *)reallocate(NULL, 0, sizeof(struct obj_native));
  obj->base.type = OBJ_NATIVE;
  obj->base.hash = 0; // native don't need hash ?
  obj->arity = arity;
  obj->method = method;

  value.as.obj = (struct object *)obj;
  return value;
}

struct value native_clock(int arity, struct value *argv)
{
  return value_make_number((double)clock() / CLOCKS_PER_SEC);
}
