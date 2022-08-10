#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "memory.h"
#include "value.h"

// FNV-1a hash function
uint32_t FNV1a_hash(const char *key, int length)
{
  uint32_t hash = 2166136261u;
  for (int i = 0; i < length; i++) {
    hash ^= (uint8_t)key[i];
    hash *= 16777619;
  }
  return hash;
}

// heap maintains a list of allocated object
static Object *heap = NULL;

void trace_heap()
{
  Object *item = heap;
  printf("===== Trace Heap Begin =====\n");
  printf("Heap Size: %d\n", mem_alloc());
  while (item) {
    if (item->marked) {
      printf("mark    ");
    } else {
      printf("        ");
    }
    object_print(item);
    printf("\n");
    item = item->next;
  }
  printf("===== Trace Heap End   =====\n");
}

void sweep_heap(void)
{
  Object **objp = &heap;
  while (*objp) {
    Object *obj = *objp;
    if ((obj)->marked != true) {
      *objp = obj->next;
      object_free(obj);
      reallocate(obj, obj->size, 0);
    } else {
      (*objp)->marked = false;
      objp = &(*objp)->next;
    }
  }
}

// object_alloc allocates size memory for new object and set up corresponding
// fields
Object *object_alloc(int size, object_t type, uint32_t hash,
                     bool (*equal_fn)(Object *, Object *),
                     void (*format)(Object *), void (*destructor)(Object *))
{
  Object *item = (Object *)reallocate(NULL, 0, size);
  item->next = heap;
  item->marked = false;
  heap = item;

  item->type = type;
  item->hash = hash;
  item->size = size;
  item->equal = equal_fn;
  item->format = format;
  item->destructor = destructor;

  return item;
}

void object_free(Object *obj) { return obj->destructor(obj); }

bool object_equal(Object *obj1, Object *obj2)
{
  if (obj1->type != obj2->type) {
    return false;
  }
  if (obj1->equal == NULL) {
    return obj1 == obj2;
  }
  return obj1->equal(obj1, obj2);
}

void object_print(Object *obj)
{
  if (obj->format == NULL) {
    printf("type should not format");
    return;
  }
  return obj->format(obj);
}

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
  return FNV1a_hash((char *)&d, sizeof(double) / sizeof(char));
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
  }
  panic("unknown type of value");
}

bool value_truable(Value value)
{
  if (is_nil(value)) {
    return false;
  } else if (is_bool(value)) {
    return value_as_bool(value);
  } else {
    return true;
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
  }
}
