#include <stddef.h>
#include <stdio.h>

#include "memory.h"
#include "value.h"

Value value_make_bool (bool boolean)
{
  Value value;
  value.Type = VT_BOOL;
  value.as.boolean = boolean;
  return value;
}

Value value_make_number (double number)
{
  Value value;
  value.Type = VT_NUM;
  value.as.number = number;
  return value;
}

Value value_make_object (OBJ *obj)
{
  Value value;
  value.Type = VT_OBJ;
  value.as.obj = obj;
  return value;
}

Value value_make_string (char *str, int len)
{
  Value value;
  value.Type = VT_OBJ;
  value.as.obj = string_copy (str, len);
  return value;
}

void value_array_init (ValueArray *va)
{
  va->len = 0;
  va->cap = 0;
  va->value = NULL;
}

void value_array_write (ValueArray *va, Value v)
{
  if (va->cap < va->len + 1)
  {
    int oldSize = va->cap;
    va->cap = grow_cap (va->cap);
    va->value = grow_array (Value, va->value, oldSize, va->cap);
  }
  va->value[va->len] = v;
  va->len++;
}

void value_array_free (ValueArray *va)
{
  free_array (Value, va->value, va->cap);
  value_array_init (va);
}

void value_print (Value v)
{
  switch (v.Type)
  {
    case VT_NIL:
      printf ("nil");
      break;
    case VT_NUM:
      printf ("%g", value_as_number (v));
      break;
    case VT_BOOL:
      if (value_as_bool (v) == true)
      {
        printf ("true");
      }
      else
      {
        printf ("false");
      }
      break;
    case VT_OBJ:
      object_print (value_as_obj (v));
  }
}
