#include <time.h>

#include "memory.h"
#include "native.h"

Value value_make_native(int arity, native_fn method)
{
  Value value;
  value.type = VT_OBJ;

  ObjectNative *obj = (ObjectNative *)reallocate(NULL, 0, sizeof(ObjectNative));
  obj->base.type = OBJ_NATIVE;
  obj->base.hash = 0; // native don't need hash ?
  obj->arity = arity;
  obj->method = method;

  value.as.obj = (Object *)obj;
  return value;
}

Value native_clock(int arity, Value *argv)
{
  return value_make_number((double)clock() / CLOCKS_PER_SEC);
}
