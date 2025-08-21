#include "core.h"
#include "../debug/dz_debug.h"

size_t TOTAL_MALLOCS = 0;

void *xmalloc(const uint32_t size) {
  void *ptr = malloc(size);
  TOTAL_MALLOCS++;
  if (!ptr || !size) {
    DZ_ERRORNO("CRITICAL: Could not allocate memory of size %" PRIu32
               "\nExiting...",
               size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *xcalloc(const uint32_t n, const uint32_t size) {
  void *ptr = calloc(n, size);
  TOTAL_MALLOCS++;
  if (!ptr || !size) {
    DZ_ERRORNO("CRITICAL: Could not allocate memory of size %" PRIu32
               ".\nExiting...",
               size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *xrealloc(void *old_ptr, const uint32_t new_size) {
  void *ptr = realloc(old_ptr, new_size);
  TOTAL_MALLOCS++;
  if (!ptr || !new_size) {
    DZ_ERRORNO("CRITICAL: Could not allocate memory of size %" PRIu32
               ". \nExiting...",
               new_size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}
