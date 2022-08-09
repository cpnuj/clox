#include <stdio.h>
#include <string.h>
#include <time.h>

#include "debug.h"
#include "memory.h"
#include "object.h"

void none_destructor(Object *obj) { return; }

void string_format(Object *obj) { printf("%s", ((ObjectString *)obj)->str); }

void string_destructor(Object *obj)
{
  ObjectString *string = (ObjectString *)obj;
  if (string->str != string->raw) {
    reallocate(string->str, string->len + 1, 0);
  }
}

Object *string_copy(char *src, int len)
{
  ObjectString *obj;
  uint32_t hash;

  // We need one more byte for trailing \0
  size_t size = sizeof(*obj) + len + 1;
  hash = FNV1a_hash(src, len);
  obj = (ObjectString *)object_alloc(size, OBJ_STRING, hash, string_equal,
                                     string_format, string_destructor);

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

  hash = FNV1a_hash(src, len);
  obj = (ObjectString *)object_alloc(sizeof(ObjectString), OBJ_STRING, hash,
                                     string_equal, string_format,
                                     none_destructor);

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
  printf("<fn prototype ");
  string_format((Object *)((ObjectFunction *)f)->name);
  printf(">");
}

bool function_equal(Object *f1, Object *f2) { return f1 == f2; }

void function_destructor(Object *obj)
{
  chunk_free(&((ObjectFunction *)obj)->chunk);
}

Object *fun_new(int arity, ObjectString *name)
{
  ObjectFunction *obj;

  obj = (ObjectFunction *)object_alloc(sizeof(ObjectFunction), OBJ_FUNCTION,
                                       name->base.hash, function_equal,
                                       function_format, function_destructor);

  obj->arity = arity;
  obj->name = name;
  chunk_init(&obj->chunk);

  return (Object *)obj;
}

void upvalue_format(Object *upvalue)
{
  printf("<upvalue %p>", ((ObjectUpValue *)upvalue)->location);
}

ObjectUpValue *upvalue_new(Value *location)
{
  ObjectUpValue *up;
  up = (ObjectUpValue *)object_alloc(sizeof(ObjectUpValue), OBJ_UPVALUE, nohash,
                                     NULL /* equal_fn */, upvalue_format,
                                     none_destructor);

  up->location = location;
  up->closed = value_make_nil();
  up->next = NULL;
  return up;
}

void upvalue_close(ObjectUpValue *to_close)
{
  to_close->closed = *to_close->location;
  to_close->location = &to_close->closed;
}

void closure_format(Object *c)
{
  printf("<fn ");
  string_format((Object *)((ObjectClosure *)c)->proto->name);
  printf(">");
}

void closure_destructor(Object *obj)
{
  ObjectClosure *closure = (ObjectClosure *)obj;
  reallocate(closure->upvalues, sizeof(ObjectUpValue *) * closure->upvalue_size,
             0);
}

bool closure_equal(Object *c1, Object *c2) { return c1 == c2; }

ObjectClosure *closure_new(ObjectFunction *proto)
{
  ObjectClosure *closure;
  closure = (ObjectClosure *)object_alloc(sizeof(ObjectClosure), OBJ_CLOSURE,
                                          proto->base.hash, closure_equal,
                                          closure_format, closure_destructor);

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
  ObjectNative *obj;
  obj = (ObjectNative *)object_alloc(sizeof(ObjectNative), OBJ_NATIVE, nohash,
                                     NULL /* equal_fn */, native_format,
                                     none_destructor);
  obj->arity = arity;
  obj->method = method;
  return obj;
}

Value native_clock(int arity, Value *argv)
{
  return value_make_number((double)clock() / CLOCKS_PER_SEC);
}

void class_format(Object *klass) { object_print(((ObjectClass *)klass)->name); }

ObjectClass *class_new(ObjectString *name)
{
  ObjectClass *klass;
  klass = (ObjectClass *)object_alloc(sizeof(ObjectClass), OBJ_CLASS, nohash,
                                      NULL, class_format, none_destructor);

  klass->name = name;
  map_init(&klass->methods);
  return klass;
}

void instance_format(Object *obj)
{
  ObjectInstance *ins = (ObjectInstance *)obj;
  class_format(ins->klass);
  printf(" instance");
}

ObjectInstance *instance_new(ObjectClass *klass)
{
  ObjectInstance *ins;
  ins = (ObjectInstance *)object_alloc(sizeof(ObjectInstance), OBJ_INSTANCE,
                                       nohash, NULL, instance_format,
                                       none_destructor);

  ins->klass = klass;
  map_init(&ins->fields);
  return ins;
}

void bound_method_format(Object *obj)
{
  ObjectBoundMethod *bm = (ObjectBoundMethod *)obj;
  closure_format(bm->method);
}

ObjectBoundMethod *bound_method_new(ObjectClosure *method, ObjectInstance *ins)
{
  ObjectBoundMethod *bm;
  bm = (ObjectBoundMethod *)object_alloc(sizeof(ObjectBoundMethod),
                                         OBJ_BOUND_METHOD, nohash, NULL,
                                         bound_method_format, none_destructor);

  bm->method = method;
  bm->receiver = ins;
  return bm;
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
