#include "error_reporter.h"
#include "stb/stb_ds.h"
#include <stdarg.h>
#include <stdio.h>

// GLOBAL SINGLETON
typedef struct ErrorReporter {
  CompilerError *errors;
} ErrorReporter;

struct ErrorReporter ERROR_REPORTER = {
    .errors = NULL,
};

// Private Methods

const char *_get_error_type_str(const ERROR_TYPE err_type) {
#define X(enum, str)                                                           \
  case enum:                                                                   \
    return str;
  switch (err_type) { ERROR_TYPE_X_VALUES }
  return "";
#undef X
}

// Public API

void er_add_error(ERROR_TYPE error, const char *file, size_t line, size_t col,
                  const char *msg, ...) {
  va_list args;
  va_start(args, msg);
  er_add_error_v(error, file, line, col, msg, args);
  va_end(args);
}

void er_add_error_v(ERROR_TYPE error, const char *file, size_t line, size_t col,
                    const char *msg, va_list args) {
  char *formatted_msg = NULL;
  const int bytes_read = vasprintf(&formatted_msg, msg, args);
  if (!bytes_read) {
    DZ_THROW("Could not vasprintf -- out of memory");
  }

  CompilerError new_error = {
      .col = col,
      .line = line,
      .message = formatted_msg,
      .file = strdup(file),
      .type = error,
  };
  arrput(ERROR_REPORTER.errors, new_error);
}

void er_print_all_errors(void) {
  for (size_t i = 0; i < arrlenu(ERROR_REPORTER.errors); i++) {
    const CompilerError err = ERROR_REPORTER.errors[i];
    fprintf(stderr,
            "%s[COMPILER ERROR]%s In file %s:%ld:%ld: %s error - %s\n\n", KRED,
            KNRM, err.file, err.line, err.col, _get_error_type_str(err.type),
            err.message);
  }
}

bool er_has_errors(void) { return arrlen(ERROR_REPORTER.errors) != 0; }

void er_free(void) {
  for (size_t i = 0; i < arrlenu(ERROR_REPORTER.errors); i++) {
    const CompilerError err = ERROR_REPORTER.errors[i];
    if (err.message) {
      free(err.message);
    }
    if (err.file) {
      free(err.file);
    }
  }
  arrfree(ERROR_REPORTER.errors);
  ERROR_REPORTER.errors = NULL;
}

#ifdef DZ_TESTING
size_t er_get_error_count(void) {
  if (!ERROR_REPORTER.errors) {
    return 0;
  }
  return arrlen(ERROR_REPORTER.errors);
}

CompilerError er_get_error_at(size_t index) {
  if (!ERROR_REPORTER.errors ||
      index >= (size_t)arrlen(ERROR_REPORTER.errors)) {
    CompilerError empty_error = {0};
    return empty_error;
  }
  return ERROR_REPORTER.errors[index];
}
#endif
