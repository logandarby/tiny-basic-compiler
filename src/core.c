#include "core.h"

void *xmalloc(const size_t size) {
  void *ptr = malloc(size);
  if (!ptr || !size) {
    fprintf(stderr,
            "CRITICAL: Could not allocate memory of size %zd. %s\nExiting...",
            size, strerror(errno));
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *xcalloc(const size_t n, const size_t size) {
  void *ptr = calloc(n, size);
  if (!ptr || !size) {
    fprintf(stderr,
            "CRITICAL: Could not allocate memory of size %zd. %s\nExiting...",
            size, strerror(errno));
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *xrealloc(void *old_ptr, const size_t new_size) {
  void *ptr = realloc(old_ptr, new_size);
  if (!ptr || !new_size) {
    fprintf(stderr,
            "CRITICAL: Could not allocate memory of size %zd. %s\nExiting...",
            new_size, strerror(errno));
    exit(EXIT_FAILURE);
  }
  return ptr;
}
