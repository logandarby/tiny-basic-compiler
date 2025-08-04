#include "test_util.h"

#include <stdio.h>
#include <stdlib.h>

typedef struct {
  char *buffer;
  size_t size;
  size_t capacity;
} capture_buffer_t;

char *capture_fd_output(int fd, void (*func)(void)) {
  int pipefd[2];
  if (pipe(pipefd) == -1)
    return NULL;

  // Save original fd
  int saved_fd = dup(fd);

  // Redirect fd to pipe
  dup2(pipefd[1], fd);
  close(pipefd[1]);

  // Run function
  func();
  fflush(fd == STDOUT_FILENO ? stdout : stderr);

  // Restore fd
  dup2(saved_fd, fd);
  close(saved_fd);

  // Read all output (handle large outputs)
  char *buffer = malloc(4096);
  size_t total_read = 0;
  size_t capacity = 4096;
  ssize_t bytes_read;

  while ((bytes_read = read(pipefd[0], buffer + total_read,
                            capacity - total_read - 1)) > 0) {
    total_read += bytes_read;
    if (total_read >= capacity - 1) {
      capacity *= 2;
      buffer = realloc(buffer, capacity);
    }
  }

  close(pipefd[0]);

  if (total_read > 0) {
    buffer[total_read] = '\0';
    return buffer;
  }

  free(buffer);
  return NULL;
}
