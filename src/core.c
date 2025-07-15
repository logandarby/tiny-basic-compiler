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
