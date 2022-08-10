#include <stddef.h>
#include <stdio.h>

#include "map.h"
#include "memory.h"

#define ITEM_FREE 0
#define ITEM_USED 1
#define ITEM_TOMB 2

#define is_free(item) (item.tag == ITEM_FREE)
#define is_used(item) (item.tag == ITEM_USED)
#define is_tomb(item) (item.tag == ITEM_TOMB)

#define MAP_INIT_SIZE 8

static void item_list_init(MapItem *items, unsigned int size)
{
  for (int i = 0; i < size; i++) {
    items[i].tag = ITEM_FREE;
  }
}

static void _map_init(Map *map, unsigned int size)
{
  map->size = size;
  map->used = 0;
  map->items = grow_array(MapItem, NULL, 0, size);
  item_list_init(map->items, map->size);
}

void map_init(Map *map) { _map_init(map, MAP_INIT_SIZE); }

// map_find finds the value for specified key.
// If found, the index of corresponding value will be return.
// Else, returns the first unused index. The returned index
// will never be invalid since our map should never be full.
static unsigned int map_find(Map *map, Value key)
{
  unsigned int idx = value_hash(key) & (map->size - 1);
  for (;;) {
    MapItem item = map->items[idx];
    if (is_free(item)) {
      return idx;
    }
    if (is_used(item) && value_equal(key, item.key)) {
      return idx;
    }
    // open addressing
    idx = (idx + 1) & (map->size - 1);
  }
}

// map_grow resizes the map's item list and rehash all the used items.
// The new size is 2 * used.
static void map_grow(Map *map)
{
  Map tmp;
  _map_init(&tmp, map->size * 2);

  for (int i = 0; i < map->size; i++) {
    MapItem item = map->items[i];
    if (item.tag == ITEM_USED) {
      map_put(&tmp, item.key, item.value);
    }
  }

  free_array(MapItem, map->items, map->size);
  map->size = tmp.size;
  map->used = tmp.used;
  map->items = tmp.items;
}

#define LOAD_FACTOR_MAX (0.9)

// map_put puts a key-value pair to the map.
void map_put(Map *map, Value key, Value value)
{
  unsigned int idx = map_find(map, key);
  if (is_free(map->items[idx])) {
    map->used++;
  }
  map->items[idx].tag = ITEM_USED;
  map->items[idx].key = key;
  map->items[idx].value = value;

  // If the load factor (used / size) is greater than LOAD_FACTOR_MAX,
  // grows the map.
  float load_factor = (float)map->used / (float)map->size;
  if (load_factor > LOAD_FACTOR_MAX) {
    map_grow(map);
  }
}

int map_del(Map *map, Value key)
{
  unsigned int idx = map_find(map, key);
  if (is_free(map->items[idx])) {
    return 0;
  }
  map->items[idx].tag = ITEM_TOMB;
  return 1;
}

int map_get(Map *map, Value key, Value *pvalue)
{
  unsigned int idx = map_find(map, key);
  MapItem item = map->items[idx];
  if (is_free(item)) {
    return 0;
  }
  if (pvalue != NULL) {
    *pvalue = item.value;
  }
  return 1;
}

MapIter *map_iter_new(Map *map)
{
  MapIter *iter = (MapIter *)reallocate(NULL, 0, sizeof(MapIter));
  iter->pos = -1;
  iter->map = map;
  iter->key = value_make_nil();
  iter->val = value_make_nil();
  return iter;
}

bool map_iter_next(MapIter *iter)
{
  while (++iter->pos < iter->map->size) {
    MapItem item = iter->map->items[iter->pos];
    if (is_used(item)) {
      iter->key = item.key;
      iter->val = item.value;
      return true;
    }
  }
  return false;
}

void map_iter_close(MapIter *iter) { reallocate(iter, sizeof(MapIter), 0); }