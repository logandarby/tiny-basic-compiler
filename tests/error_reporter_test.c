#include "../src/common/error_reporter.h"
#include <criterion/criterion.h>
#include <string.h>
#include <unistd.h>

#include "test_util.h"

Test(error_reporter, basic_error_reporting) {
  // Add multiple different error types
  er_add_error(ERROR_LEXICAL, "test.c", 10, 5, "Unexpected character '%c'",
               '$');
  er_add_error(ERROR_SEMANTIC, "main.c", 25, 12, "Undefined variable '%s'",
               "count");
  er_add_error(ERROR_GRAMMAR, "parser.c", 42, 8, "Expected '%s' but found '%s'",
               "THEN", "ELSE");
  er_add_error(ERROR_LEXICAL, "input.txt", 1, 1, "Invalid number format");
  er_add_error(ERROR_SEMANTIC, "test.c", 15, 20, "Type mismatch in expression");

  // Check that errors were added
  cr_assert(er_has_errors(), "Should have errors after adding multiple");

  // Print errors to verify output
  char *output = capture_fd_output(STDERR_FILENO, er_print_all_errors);
  cr_assert_not_null(strstr(output, "test.c:10:5"),
                     "Error output contains correct filename with column and "
                     "line information");
  cr_assert_not_null(strstr(output, "main.c:25:12"),
                     "Error output contains correct filename with column and "
                     "line information");
  cr_assert_not_null(strstr(output, "parser.c:42:8"),
                     "Error output contains correct filename with column and "
                     "line information");
  cr_assert_not_null(strstr(output, "input.txt:1:1"),
                     "Error output contains correct filename with column and "
                     "line information");
  free(output);

  // Clean up
  er_free();
}
