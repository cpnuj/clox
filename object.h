#ifndef clox_object_h
#define clox_object_h

#include <stdint.h>

enum
{
  OBJ_STRING = 1,
};

typedef struct _obj
{
  int type;
  uint32_t hash;
} OBJ;

void object_init (OBJ *obj, int type, uint32_t hash);

#define object_is(obj, t) (obj->type == t)
#define object_as(obj, t) ((t *)obj)

// struct obj_string represents a string object in clox.
// If the raw string is embeded within this struct, pointer str points to filed
// raw, else str may point to other space specified by user.
struct obj_string
{
  OBJ base;
  int len;
  char *str;
  char raw[];
};

OBJ *string_copy (char *, int);
OBJ *string_take (char *, int);

OBJ *string_concat (struct obj_string *, struct obj_string *);

void object_print (OBJ *);

#endif
