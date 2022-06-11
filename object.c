#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"

void object_init(struct object *obj, int type, uint32_t hash)
{
  obj->type = type;
  obj->hash = hash;
}

// FNV-1a hash function
static uint32_t string_hash(const char *key, int length)
{
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

struct object *string_copy(char *src, int len)
{
  struct obj_string *obj;
  uint32_t hash;

  // We need one more byte for trailing \0
  size_t size = sizeof(*obj) + len + 1;
  obj = (struct obj_string *)reallocate(NULL, 0, size);
  hash = string_hash(src, len);

  object_init((struct object *)obj, OBJ_STRING, hash);

  memcpy(obj->raw, src, len);
  obj->raw[len] = '\0';

  obj->len = len;
  obj->str = obj->raw;
  return (struct object *)obj;
}

struct object *string_take(char *src, int len)
{
  struct obj_string *obj;
  uint32_t hash;

  obj = (struct obj_string *)reallocate(NULL, 0, sizeof(struct obj_string));
  hash = string_hash(src, len);

  object_init((struct object *)obj, OBJ_STRING, hash);

  obj->len = len;
  obj->str = src;
  return (struct object *)obj;
}

struct object *string_concat(struct obj_string *obj1, struct obj_string *obj2)
{
  struct object *obj;
  char *dst;
  int len;

  len = obj1->len + obj2->len;
  dst = (char *)reallocate(NULL, 0, len + 1);
  strcpy(dst, obj1->str);
  strcpy(dst + obj1->len, obj2->str);

  obj = string_take(dst, len);
  return obj;
}

void object_print(struct object *obj)
{
  switch (obj->type) {
  case OBJ_STRING:
    printf("%s", ((struct obj_string *)obj)->str);
    break;
  }
}
