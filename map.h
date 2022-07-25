#ifndef clox_map_h
#define clox_map_h

#include "value.h"

typedef struct {
  uint8_t tag;
  Value key;
  Value value;
} MapItem;

typedef struct {
  unsigned int size;
  unsigned int used;

  MapItem *items;
} Map;

void map_init(Map *);
void map_put(Map *, Value, Value);
int map_del(Map *, Value);
int map_get(Map *, Value, Value *);

#endif