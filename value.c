#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "memory.h"
#include "value.h"

Value value_make_nil()
{
  Value value;
  value.type = VT_NIL;
  return value;
}

Value value_make_bool(bool boolean)
{
  Value value;
  value.type = VT_BOOL;
  value.as.boolean = boolean;
  return value;
}

Value value_make_number(double number)
{
  Value value;
  value.type = VT_NUM;
  value.as.number = number;
  return value;
}

Value value_make_object(Object *obj)
{
  Value value;
  value.type = VT_OBJ;
  value.as.obj = obj;
  return value;
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

void value_array_init(ValueArray *va)
{
  va->len = 0;
  va->cap = 0;
  va->value = NULL;
}

void value_array_write(ValueArray *va, Value v)
{
  if (va->cap < va->len + 1) {
    int oldSize = va->cap;
    va->cap = grow_cap(va->cap);
    va->value = grow_array(Value, va->value, oldSize, va->cap);
  }
  va->value[va->len] = v;
  va->len++;
}

void value_array_free(ValueArray *va)
{
  free_array(Value, va->value, va->cap);
  value_array_init(va);
}

Value value_array_get(ValueArray *vlist, int idx)
{
  if (idx >= vlist->len || idx < 0)
    panic("invalid reference index of value array");

  return *(vlist->value + idx);
}

uint32_t hash_double(double d)
{
  return string_hash((char *)&d, sizeof(double) / sizeof(char));
}

uint32_t value_hash(Value value)
{
  if (is_nil(value)) {
    return 0;
  } else if (is_bool(value)) {
    return (value_as_bool(value) == true) ? 1 : 0;
  } else if (is_number(value)) {
    return hash_double(value_as_number(value));
  } else if (is_object(value)) {
    return value_as_obj(value)->hash;
  } else if (is_ident(value)) {
    return value_as_obj(value)->hash;
  }
  panic("unknown type of value");
}

bool value_is_false(Value value)
{
  if (is_nil(value)) {
    return true;
  } else if (is_bool(value)) {
    return value_as_bool(value) == false;
  } else {
    return false;
  }
}

bool value_equal(Value v1, Value v2)
{
  if (v1.type != v2.type)
    return false;
  // v1 and v2 have same type now.
  switch (v1.type) {
  case VT_NIL:
    return true;
  case VT_BOOL:
    return value_as_bool(v1) == value_as_bool(v2);
  case VT_NUM:
    return value_as_number(v1) == value_as_number(v2);
  case VT_OBJ:
  case VT_IDENT:
    return object_equal(value_as_obj(v1), value_as_obj(v2));
  default:
    panic("unknown value type");
  }
}

void value_print(Value v)
{
  switch (v.type) {
  case VT_NIL:
    printf("nil");
    break;
  case VT_NUM:
    printf("%g", value_as_number(v));
    break;
  case VT_BOOL:
    if (value_as_bool(v) == true) {
      printf("true");
    } else {
      printf("false");
    }
    break;
  case VT_OBJ:
    object_print(value_as_obj(v));
    break;
  case VT_IDENT:
    printf("ident ");
    object_print(value_as_obj(v));
    break;
  }
}
