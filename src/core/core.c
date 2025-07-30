#include "core.h"
#include "../debug/dz_debug.h"

void *xmalloc(const size_t size) {
  void *ptr = malloc(size);
  if (!ptr || !size) {
    DZ_ERRORNO("CRITICAL: Could not allocate memory of size %zd\nExiting...",
               size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *xcalloc(const size_t n, const size_t size) {
  void *ptr = calloc(n, size);
  if (!ptr || !size) {
    DZ_ERRORNO("CRITICAL: Could not allocate memory of size %zd.\nExiting...",
               size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *xrealloc(void *old_ptr, const size_t new_size) {
  void *ptr = realloc(old_ptr, new_size);
  if (!ptr || !new_size) {
    DZ_ERRORNO("CRITICAL: Could not allocate memory of size %zd. \nExiting...",
               new_size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}
