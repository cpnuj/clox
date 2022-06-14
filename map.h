#ifndef clox_map_h
#define clox_map_h

#include "value.h"

struct map_item {
  uint8_t tag;
  struct value key;
  struct value value;
};

struct map {
  unsigned int size;
  unsigned int used;

  struct map_item *items;
};

void map_init(struct map *);
void map_put(struct map *, struct value, struct value);
int map_del(struct map *, struct value);
int map_get(struct map *, struct value, struct value *);

#endif