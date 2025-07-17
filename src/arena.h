#pragma once

// ----------------------------------
// ARENA ALLOCATOR
//
// Dynamically Resizing Arena Allocation
//
// This implementation is partially borrowed from
// https://github.com/spevnev/uprintf
// ----------------------------------

#include "core.h"

// Arena type with forward declaration for internal region
typedef struct _ArenaRegion ArenaRegion;

typedef struct Arena {
  ArenaRegion *head;
} Arena;

// Init an arena
Arena arena_init(void);

// Allocates memory from the arena
void *arena_alloc(Arena *a, size_t size);

// Frees the entire arena and all its regions
void arena_destroy(Arena *a);

// Allocates a string from arena, copying from [begin, end)
char *arena_allocate_string(Arena *a, const char *begin, const char *end);

// Concatenates multiple strings in arena (use NULL to terminate the list)
char *_arena_concat(Arena *a, ...);

// Convenience macro for arena_concat - automatically adds NULL terminator
#define arena_concat(a, ...) _arena_concat((a), __VA_ARGS__, NULL)
