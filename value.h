#ifndef clox_value_h
#define clox_value_h

#include <stdbool.h>
#include <stdint.h>

#include "object.h"

typedef enum {
  VT_NIL,
  VT_NUM,
  VT_BOOL,
  VT_OBJ,
  VT_IDENT,
} value_t;

struct value {
  value_t type;
  union {
    bool boolean;
    double number;
    struct object *obj;
  } as;
};

#define value_as_bool(value) (value.as.boolean)
#define value_as_number(value) (value.as.number)
#define value_as_obj(value) (value.as.obj)

#define is_nil(value) (value.type == VT_NIL)
#define is_number(value) (value.type == VT_NUM)
#define is_bool(value) (value.type == VT_BOOL)
#define is_object(value) (value.type == VT_OBJ)
#define is_ident(value) (value.type == VT_IDENT)

#define is_string(value)                                                       \
  (is_object(value) && object_is(value_as_obj(value), OBJ_STRING))

#define is_fun(value)                                                          \
  (is_object(value) && object_is(value_as_obj(value), OBJ_FUN))

// Macros cast value to specific object
#define value_as_string(value)                                                 \
  (object_as(value_as_obj(value), struct obj_string))

struct value value_make_nil(void);
struct value value_make_bool(bool);
struct value value_make_number(double);
struct value value_make_object(struct object *);
struct value value_make_ident(char *, int);
struct value value_make_string(char *, int);
struct value value_make_fun(int, struct obj_string *);

uint32_t value_hash(struct value);
bool value_is_false(struct value);
bool value_equal(struct value, struct value);
void value_print(struct value v);

struct value_list {
  int len;
  int cap;
  struct value *value;
};

void value_array_init(struct value_list *va);
void value_array_write(struct value_list *va, struct value v);
void value_array_free(struct value_list *va);
struct value value_array_get(struct value_list *vlist, int idx);

#endif
