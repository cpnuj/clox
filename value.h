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
} value_t;

typedef enum {
  OBJ_STRING = 1,
  OBJ_FUNCTION,
  OBJ_UPVALUE,
  OBJ_CLOSURE,
  OBJ_NATIVE,
  OBJ_CLASS,
  OBJ_INSTANCE,
} object_t;

typedef struct Object {
  object_t type;
  uint32_t hash;

  // size of one object alloc
  int size;

  // marked is used in gc
  bool marked;

  // equal points to a function returning whether two objects are equal
  bool (*equal)(struct Object *, struct Object *);

  // format points to a function to print the object
  void (*format)(struct Object *);

  // destructor is called when the object is freed
  void (*destructor)(struct Object *);

  // next is the next object on our object heap
  struct Object *next;

} Object;

void trace_heap(void);

void sweep_heap(void);

Object *object_alloc(int size, object_t type, uint32_t hash,
                     bool (*equal_fn)(Object *, Object *),
                     void (*format)(Object *), void (*destrutor)(Object *));

void object_free(Object *);
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
