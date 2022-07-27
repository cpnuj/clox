#include <stdio.h>
#include <string.h>
#include <time.h>

#include "debug.h"
#include "memory.h"
#include "object.h"

void string_format(Object *obj) { printf("%s", ((ObjectString *)obj)->str); }

Object *string_copy(char *src, int len)
{
  ObjectString *obj;
  uint32_t hash;

  // We need one more byte for trailing \0
  size_t size = sizeof(*obj) + len + 1;
  obj = (ObjectString *)reallocate(NULL, 0, size);
  hash = FNV1a_hash(src, len);

  object_init((Object *)obj, OBJ_STRING, hash, string_equal, string_format);

  memcpy(obj->raw, src, len);
  obj->raw[len] = '\0';

  obj->len = len;
  obj->str = obj->raw;
  return (Object *)obj;
}

Object *string_take(char *src, int len)
{
  ObjectString *obj;
  uint32_t hash;

  obj = (ObjectString *)reallocate(NULL, 0, sizeof(ObjectString));
  hash = FNV1a_hash(src, len);

  object_init((Object *)obj, OBJ_STRING, hash, string_equal, string_format);

  obj->len = len;
  obj->str = src;
  return (Object *)obj;
}

Object *string_concat(ObjectString *obj1, ObjectString *obj2)
{
  Object *obj;
  char *dst;
  int len;

  len = obj1->len + obj2->len;
  dst = (char *)reallocate(NULL, 0, len + 1);
  strcpy(dst, obj1->str);
  strcpy(dst + obj1->len, obj2->str);

  obj = string_take(dst, len);
  return obj;
}

bool string_equal(Object *s1, Object *s2)
{
  if (s1 == s2)
    return true;
  return strcmp(((ObjectString *)s1)->str, ((ObjectString *)s2)->str) == 0;
}

void function_format(Object *f)
{
  printf("<fn ");
  string_format((Object *)((ObjectFunction *)f)->name);
  printf(">");
}

bool function_equal(Object *f1, Object *f2) { return f1 == f2; }

Object *fun_new(int arity, ObjectString *name)
{
  ObjectFunction *obj;

  obj = (ObjectFunction *)reallocate(NULL, 0, sizeof(ObjectFunction));
  object_init((Object *)obj, OBJ_FUNCTION, name->base.hash, function_equal,
              function_format);

  obj->arity = arity;
  obj->name = name;
  chunk_init(&obj->chunk);

  return (Object *)obj;
}

ObjectUpValue *upvalue_new(Value *location)
{
  ObjectUpValue *up
      = (ObjectUpValue *)reallocate(NULL, 0, sizeof(ObjectUpValue));
  object_init((Object *)up, OBJ_UPVALUE, nohash, NULL /* equal_fn */,
              NULL /* formater */);

  up->location = location;
  up->closed = false;
  up->next = NULL;
  return up;
}

// TODO: make Value closed inside ObjectUpValue
void upvalue_close(ObjectUpValue *to_close, Value *new_location)
{
  to_close->closed = true;
  *new_location = *to_close->location;
  to_close->location = new_location;
}

void closure_format(Object *c)
{
  function_format((Object *)((ObjectClosure *)c)->proto);
}

bool closure_equal(Object *c1, Object *c2) { return c1 == c2; }

ObjectClosure *closure_new(ObjectFunction *proto)
{
  ObjectClosure *closure
      = (ObjectClosure *)reallocate(NULL, 0, sizeof(ObjectClosure));
  object_init((Object *)closure, OBJ_CLOSURE, proto->base.hash, closure_equal,
              closure_format);

  closure->proto = proto;
  closure->upvalue_size = proto->upvalue_size;
  closure->upvalues = (ObjectUpValue **)reallocate(
      NULL, 0, sizeof(ObjectUpValue *) * closure->upvalue_size);
  for (int i = 0; i < closure->upvalue_size; i++) {
    closure->upvalues[i] = NULL;
  }

  return closure;
}

void native_format(Object *native) { printf("<native fn>"); }

ObjectNative *native_new(int arity, native_fn method)
{
  ObjectNative *obj = (ObjectNative *)reallocate(NULL, 0, sizeof(ObjectNative));
  object_init((Object *)obj, OBJ_NATIVE, nohash, NULL /* equal_fn */,
              native_format);
  obj->arity = arity;
  obj->method = method;
  return obj;
}

Value native_clock(int arity, Value *argv)
{
  return value_make_number((double)clock() / CLOCKS_PER_SEC);
}

// value_make_ident makes a value with object string but as type VT_IDENT.
Value value_make_ident(char *str, int len)
{
  Value value = value_make_string(str, len);
  value.type = VT_IDENT;
  return value;
}

Value value_make_string(char *str, int len)
{
  Value value;
  value.type = VT_OBJ;
  value.as.obj = string_copy(str, len);
  return value;
}

Value value_make_fun(int arity, ObjectString *name)
{
  Value value;
  value.type = VT_OBJ;
  value.as.obj = fun_new(arity, name);
  return value;
}

Value value_make_closure(ObjectFunction *proto)
{
  return value_make_object((Object *)closure_new(proto));
}

Value value_make_native(int arity, native_fn method)
{
  Value value;
  value.type = VT_OBJ;
  value.as.obj = (Object *)native_new(arity, method);
  return value;
}
