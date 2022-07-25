#ifndef clox_object_h
#define clox_object_h

#include <stdbool.h>
#include <stdint.h>

#include "chunk.h"

enum {
  OBJ_STRING = 1,
  OBJ_FUNCTION,
  OBJ_UPVALUE,
  OBJ_CLOSURE,
  OBJ_NATIVE,
};

typedef struct {
  int type;
  uint32_t hash;
} Object;

void object_init(Object *obj, int type, uint32_t hash);
bool object_equal(Object *, Object *);

#define object_is(obj, t) (obj->type == t)
#define object_as(obj, t) ((t *)obj)

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
uint32_t string_hash(const char *, int);
bool string_equal(ObjectString *, ObjectString *);

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

typedef struct {
  Object base;
  Value *location;
} ObjectUpValue;

ObjectUpValue *upvalue_new(Value *);

// ObjectClosure is created as a running function object in runtime
typedef struct {
  Object base;
  ObjectFunction *proto;
  int upvalue_size;
  ObjectUpValue **upvalues;
} ObjectClosure;

ObjectClosure *closure_new(ObjectFunction *);

void object_print(Object *);

#endif
