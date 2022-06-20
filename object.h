#ifndef clox_object_h
#define clox_object_h

#include <stdbool.h>
#include <stdint.h>

enum {
  OBJ_STRING = 1,
};

struct object {
  int type;
  uint32_t hash;
};

void object_init(struct object *obj, int type, uint32_t hash);
bool object_equal(struct object *, struct object *);

#define object_is(obj, t) (obj->type == t)
#define object_as(obj, t) ((t *)obj)

// struct obj_string represents a string object in clox.
// If the raw string is embeded within this struct, pointer str points to filed
// raw, else str may point to other space specified by user.
struct obj_string {
  struct object base;
  int len;
  char *str;
  char raw[];
};

struct object *string_copy(char *, int);
struct object *string_take(char *, int);

struct object *string_concat(struct obj_string *, struct obj_string *);
uint32_t string_hash(const char *, int);
bool string_equal(struct obj_string *, struct obj_string *);

void object_print(struct object *);

#endif
