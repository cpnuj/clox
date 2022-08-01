#ifndef clox_memory_h
#define clox_memory_h

#define grow_cap(cap) ((cap) < 8 ? 8 : (cap)*2)

#define grow_array(type, ptr, oldCap, newCap)                                  \
  ((type *)reallocate(ptr, sizeof(type) * (oldCap), sizeof(type) * (newCap)))

#define free_array(type, ptr, size) (reallocate(ptr, sizeof(type) * (size), 0))

void *reallocate(void *ptr, int oldSize, int newSize);
int mem_alloc(void);

#endif
