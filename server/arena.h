#pragma once

// An arena is a bump allocator: a growable buffer with a count of used bytes.
// Allocating means incrementing the used bytes.
// There is no piece-wise deallocation.
// The arena can be reset in a single intruction by setting used to zero.
// When allocating, we grow the arena to twice its current size until the needed bytes fit.
// When allocating, return indexes rather than pointers, so that the values don't change on growth.

#include <stdint.h>

#define arena_get_ptr(arena, index) ((index) == (unsigned)-1 ? 0 : (arena)->buffer + (index))

typedef struct Arena {
  uint8_t *buffer;    // contiguous storage
  unsigned capacity;  // total capacity
  unsigned used;      // currently used
} Arena;

Arena* arena_build(unsigned capacity);
void arena_destroy(Arena* arena);
void arena_reset(Arena* arena);

// Store pointer into arena, return index
unsigned arena_store(Arena* arena, const uint8_t *src, unsigned len);

// Store length + value into arena, return index.
unsigned arena_store_framed(Arena* arena, const uint8_t *src, unsigned len);
