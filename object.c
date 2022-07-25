#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "memory.h"
#include "object.h"

void object_init(Object *obj, int type, uint32_t hash)
{
  obj->type = type;
  obj->hash = hash;
}

bool object_equal(Object *obj1, Object *obj2)
{
  if (obj1->type != obj2->type)
    return false;
  switch (obj1->type) {
  case OBJ_STRING:
    return string_equal(object_as(obj1, ObjectString),
                        object_as(obj2, ObjectString));
  case OBJ_FUNCTION:
    return string_equal(object_as(obj1, ObjectFunction)->name,
                        object_as(obj2, ObjectFunction)->name);
  default:
    panic("unknown object type")
  }
}

// FNV-1a hash function
uint32_t string_hash(const char *key, int length)
{
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

Object *string_copy(char *src, int len)
{
  ObjectString *obj;
  uint32_t hash;

  // We need one more byte for trailing \0
  size_t size = sizeof(*obj) + len + 1;
  obj = (ObjectString *)reallocate(NULL, 0, size);
  hash = string_hash(src, len);

  object_init((Object *)obj, OBJ_STRING, hash);

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
  hash = string_hash(src, len);

  object_init((Object *)obj, OBJ_STRING, hash);

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

bool string_equal(ObjectString *s1, ObjectString *s2)
{
  if (s1 == s2)
    return true;
  return strcmp(s1->str, s2->str) == 0;
}

Object *fun_new(int arity, ObjectString *name)
{
  ObjectFunction *obj;

  obj = (ObjectFunction *)reallocate(NULL, 0, sizeof(ObjectFunction));
  object_init((Object *)obj, OBJ_FUNCTION, name->base.hash);

  obj->arity = arity;
  obj->name = name;
  chunk_init(&obj->chunk);

  return (Object *)obj;
}

ObjectUpValue *upvalue_new(Value *location)
{
  ObjectUpValue *up
      = (ObjectUpValue *)reallocate(NULL, 0, sizeof(ObjectUpValue));
  object_init((Object *)up, OBJ_UPVALUE, nohash);
  up->location = location;
  return up;
}

ObjectClosure *closure_new(ObjectFunction *proto)
{
  ObjectClosure *closure
      = (ObjectClosure *)reallocate(NULL, 0, sizeof(ObjectClosure));
  object_init((Object *)closure, OBJ_CLOSURE, proto->base.hash);

  closure->proto = proto;
  closure->upvalue_size = proto->upvalue_size;
  closure->upvalues = (ObjectUpValue **)reallocate(
      NULL, 0, sizeof(ObjectUpValue *) * closure->upvalue_size);
  for (int i = 0; i < closure->upvalue_size; i++) {
    closure->upvalues[i] = NULL;
  }

  return closure;
}

void object_print(Object *obj)
{
  switch (obj->type) {
  case OBJ_STRING:
    printf("%s", ((ObjectString *)obj)->str);
    break;
  case OBJ_FUNCTION:
    printf("<fn %s>", ((ObjectFunction *)obj)->name->str);
    break;
  case OBJ_CLOSURE:
    printf("<fn %s>", ((ObjectClosure *)obj)->proto->name->str);
    break;
  case OBJ_NATIVE:
    printf("<native fn>");
    break;
  default:
    panic("unknown object type")
  }
}
