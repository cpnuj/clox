#ifndef clox_memory_h
#define clox_memory_h

#define GROW_CAP(cap) ((cap) < 8 ? 8 : (cap)*2)

#define GROW_ARRAY(type, ptr, oldCap, newCap)                                 \
  ((type *)reallocate (ptr, sizeof (type) * (oldCap),                         \
                       sizeof (type) * (newCap)))

#define FREE_ARRAY(type, ptr, size)                                           \
  (reallocate (ptr, sizeof (type) * (size), 0))

void *reallocate (void *ptr, int oldSize, int newSize);

#endif
