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
    va->cap = GROW_CAP (va->cap);
    va->value = GROW_ARRAY (Value, va->value, oldSize, va->cap);
  }
  va->value[va->len] = v;
  va->len++;
}

void value_array_free (ValueArray *va)
{
  FREE_ARRAY (Value, va->value, va->cap);
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
      printf ("%g", VALUE_AS_NUMBER (v));
      break;
    case VT_BOOL:
      if (VALUE_AS_BOOL (v) == true)
      {
        printf ("true");
      }
      else
      {
        printf ("false");
      }
      break;
  }
}
