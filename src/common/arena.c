#include "arena.h"
#include "dz_debug.h"

#define INITIAL_ARENA_SIZE 4000 // 4kb

typedef struct _ArenaRegion {
  uint8_t *data;
  size_t capacity;
  size_t length;
  struct _ArenaRegion *prev;
} ArenaRegion;

Arena arena_init() {
  Arena a = {
      .head = NULL,
  };
  return a;
}

static ArenaRegion *_arena_alloc_region(size_t capacity, ArenaRegion *prev) {
  ArenaRegion *region = (ArenaRegion *)xmalloc(sizeof(*region));
  region->capacity = capacity;
  region->data = (uint8_t *)xmalloc(region->capacity * sizeof(*region->data));
  region->length = 0;
  region->prev = prev;
  return region;
}

void *arena_alloc(Arena *a, size_t size) {
  DZ_ASSERT(a != NULL);

  if (a->head == NULL) {
    size_t capacity = INITIAL_ARENA_SIZE;
    if (size > capacity)
      capacity = size;
    a->head = _arena_alloc_region(capacity, NULL);
  }

  size_t alignment = a->head->length % sizeof(void *);
  if (alignment > 0)
    alignment = sizeof(void *) - alignment;

  if (alignment + size > a->head->capacity - a->head->length) {
    size_t new_capacity = a->head->capacity * 2;
    if (size > new_capacity)
      new_capacity = size;
    a->head = _arena_alloc_region(new_capacity, a->head);
    alignment = 0;
  }

  void *memory = a->head->data + a->head->length + alignment;
  a->head->length += alignment + size;
  return memory;
}

void arena_destroy(Arena *a) {
  if (a == NULL || a->head == NULL)
    return;

  ArenaRegion *region = a->head;
  while (region != NULL) {
    ArenaRegion *prev = region->prev;

    free(region->data);
    free(region);

    region = prev;
  }

  a->head = NULL;
}

// Copies [begin, end) to arena-allocated string
char *arena_allocate_string(Arena *a, const char *begin, const char *end) {
  DZ_ASSERT(a != NULL && begin != NULL && end != NULL);
  DZ_ASSERT(end >= begin);

  size_t len = (size_t)(end - begin);

  char *string = (char *)arena_alloc(a, len + 2);
  memcpy(string, begin, len);
  string[len] = '\0';

  return string;
}

char *_arena_concat(Arena *a, ...) {
  DZ_ASSERT(a != NULL);

  va_list va_args;

  size_t size = 0;
  va_start(va_args, a);
  while (true) {
    const char *str = va_arg(va_args, const char *);
    if (str == NULL)
      break;
    size += strlen(str);
  }
  va_end(va_args);

  char *result = arena_alloc(a, size + 1);
  result[size] = '\0';

  char *p = result;
  va_start(va_args, a);
  while (true) {
    const char *str = va_arg(va_args, const char *);
    if (str == NULL)
      break;

    size_t len = strlen(str);
    memcpy(p, str, len);
    p += len;
  }
  va_end(va_args);

  return result;
}

#define arena_concat(a, ...) _arena_concat((a), __VA_ARGS__, NULL)
