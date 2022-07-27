#ifndef clox_value_h
#define clox_value_h

#include <stdbool.h>
#include <stdint.h>

// TODO: move to some common module
uint32_t FNV1a_hash(const char *, int);

typedef enum {
  VT_NIL,
  VT_NUM,
  VT_BOOL,
  VT_OBJ,
  VT_IDENT,
} value_t;

typedef enum {
  OBJ_STRING = 1,
  OBJ_FUNCTION,
  OBJ_UPVALUE,
  OBJ_CLOSURE,
  OBJ_NATIVE,
} object_t;

typedef struct Object {
  object_t type;
  uint32_t hash;
  bool (*equal)(struct Object *, struct Object *);
  void (*format)(struct Object *);
} Object;

void object_init(Object *obj, object_t type, uint32_t hash,
                 bool (*)(Object *, Object *), void (*)(Object *));
bool object_equal(Object *, Object *);
void object_print(Object *);

#define object_is(obj, t) (obj->type == t)
#define object_as(obj, t) ((t *)obj)

struct Value {
  value_t type;
  union {
    bool boolean;
    double number;
    Object *obj;
  } as;
};

typedef struct Value Value;

#define value_as_bool(value) (value.as.boolean)
#define value_as_number(value) (value.as.number)
#define value_as_obj(value) (value.as.obj)

#define is_nil(value) (value.type == VT_NIL)
#define is_number(value) (value.type == VT_NUM)
#define is_bool(value) (value.type == VT_BOOL)
#define is_object(value) (value.type == VT_OBJ)

Value value_make_nil(void);
Value value_make_bool(bool);
Value value_make_number(double);
Value value_make_object(Object *);

uint32_t value_hash(Value);
bool value_is_false(Value);
bool value_equal(Value, Value);
void value_print(Value v);

typedef struct {
  int len;
  int cap;
  Value *value;
} ValueArray;

void value_array_init(ValueArray *va);
void value_array_write(ValueArray *va, Value v);
void value_array_free(ValueArray *va);
Value value_array_get(ValueArray *vlist, int idx);

#endif
