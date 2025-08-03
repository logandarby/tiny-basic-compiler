#pragma once

#include "../core/core.h"
#include <stdarg.h>

#define ERROR_TYPE_X_VALUES                                                    \
  X(ERROR_LEXICAL, "Lexical")                                                  \
  X(ERROR_SEMANTIC, "Semantic")                                                \
  X(ERROR_GRAMMAR, "Grammar")

typedef enum {
#define X(a, b) a,
  ERROR_TYPE_X_VALUES
#undef X
} ERROR_TYPE;

typedef struct {
  ERROR_TYPE type;
  size_t line;
  size_t col;
  char *message;
  char *file;
} CompilerError;

// Error Reporter is a Global Singelton
// Which is interfaced with these methods
struct ErrorReporter;
extern struct ErrorReporter ERROR_REPORTER;

// Public API

// Add an error. Takes a printf style format string.
void er_add_error(ERROR_TYPE error, const char *file, size_t line, size_t col,
                  const char *msg, ...) FORMAT_PRINTF(5, 6);

// Add an error. Takes a va_list for variadic arguments.
void er_add_error_v(ERROR_TYPE error, const char *file, size_t line, size_t col,
                    const char *msg, va_list args);

void er_print_all_errors(void);

bool er_has_errors(void);

// Must be called ONLY at the very end of the program
void er_free(void);

#ifdef DZ_TESTING
// Testing-only functions to access error details
size_t er_get_error_count(void);
CompilerError er_get_error_at(size_t index);
#endif
