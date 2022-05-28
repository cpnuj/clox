#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "object.h"

void object_init (OBJ *obj, int type) { obj->type = type; }

OBJ *string_copy (char *src, int len)
{
  struct obj_string *obj;

  // We need one more byte for trailing \0
  size_t size = sizeof (*obj) + len + 1;
  obj = (struct obj_string *)reallocate (NULL, 0, size);

  object_init ((OBJ *)obj, OBJ_STRING);

  memcpy (obj->raw, src, len);
  obj->raw[len] = '\0';

  obj->len = len;
  obj->str = obj->raw;
  return (OBJ *)obj;
}

OBJ *string_take (char *src, int len)
{
  struct obj_string *obj;

  obj = (struct obj_string *)reallocate (NULL, 0, sizeof (struct obj_string));

  object_init ((OBJ *)obj, OBJ_STRING);

  obj->len = len;
  obj->str = src;
  return (OBJ *)obj;
}

OBJ *string_concat (struct obj_string *obj1, struct obj_string *obj2)
{
  OBJ *obj;
  char *dst;
  int len;

  len = obj1->len + obj2->len;
  dst = (char *)reallocate (NULL, 0, len + 1);
  strcpy (dst, obj1->str);
  strcpy (dst + obj1->len, obj2->str);

  obj = string_take (dst, len);
  return obj;
}

void object_print (OBJ *obj)
{
  switch (obj->type)
  {
    case OBJ_STRING:
      printf ("%s", ((struct obj_string *)obj)->str);
      break;
  }
}
