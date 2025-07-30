#include "../src/common/error_reporter.h"
#include <criterion/criterion.h>
#include <stdio.h>

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
  printf("\n--- Testing multiple error output ---\n");
  er_print_all_errors();
  printf("--- End error output ---\n");

  // Clean up
  er_free();
}