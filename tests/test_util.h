#pragma once

// --------------------------------------
// TESTING UTIL
//
// Contains some common testing util for convenience
// --------------------------------------

#include <fcntl.h>
#include <unistd.h>

// Used for temporarily muting STDOUT and STDERR from within tests within a
// block of code. The code must be scoped after using MUTED_STDOUT. Example:
//  MUTED_OUTPUT {
//    printf("hello!") // Will not be shown
//  }
#define MUTED_OUTPUT                                                           \
  for (int _i = 0, _saved_stdout = dup(STDOUT_FILENO),                         \
           _saved_stderr = dup(STDERR_FILENO),                                 \
           _devnull = open("/dev/null", O_WRONLY);                             \
       _i == 0 && (_devnull >= 0) &&                                           \
       (dup2(_devnull, STDOUT_FILENO), dup2(_devnull, STDERR_FILENO),          \
        close(_devnull), 1);                                                   \
       _i++, dup2(_saved_stdout, STDOUT_FILENO), close(_saved_stdout),         \
           dup2(_saved_stderr, STDERR_FILENO), close(_saved_stderr))

// Captures either stdout or stderr (whatever fd) and puts it into a string. .
// User must free the string after
char *capture_fd_output(int fd, void (*func)(void));
