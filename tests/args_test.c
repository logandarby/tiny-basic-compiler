#include "../src/core/args.h"
#include "test_util.h"
#include <criterion/criterion.h>

// =========================
// Filename Argument Tests
// =========================

Test(args, parse_filename_argument) {
  const char *argv[] = {"teeny", "test.basic"};
  const int argc = 2;

  Args args = parse_args(argc, argv);

  cr_assert_not_null(args, "Args should not be null");
  cr_assert(args_is_filename(args), "Should detect filename argument");
  cr_assert(!args_is_code_literal(args), "Should not detect code literal");
  cr_assert_str_eq(args_get_filename(args), "test.basic",
                   "Should return correct filename");

  args_free(&args);
  cr_assert_null(args, "Args should be null after freeing");
}

Test(args, parse_filename_with_path) {
  const char *argv[] = {"teeny", "examples/hello.basic"};
  const int argc = 2;

  Args args = parse_args(argc, argv);

  cr_assert_not_null(args, "Args should not be null");
  cr_assert(args_is_filename(args), "Should detect filename argument");
  cr_assert_str_eq(args_get_filename(args), "examples/hello.basic",
                   "Should return correct filename with path");

  args_free(&args);
}

// =========================
// Code Literal Argument Tests
// =========================

Test(args, parse_code_literal_argument) {
  const char *argv[] = {"teeny", "-c", "PRINT \"Hello World\""};
  const int argc = 3;

  Args args = parse_args(argc, argv);

  cr_assert_not_null(args, "Args should not be null");
  cr_assert(args_is_code_literal(args), "Should detect code literal argument");
  cr_assert(!args_is_filename(args), "Should not detect filename");
  cr_assert_str_eq(args_get_code_literal(args), "PRINT \"Hello World\"",
                   "Should return correct code literal");

  args_free(&args);
}

Test(args, parse_empty_code_literal) {
  const char *argv[] = {"teeny", "-c", ""};
  const int argc = 3;

  Args args = parse_args(argc, argv);

  cr_assert_not_null(args, "Args should not be null");
  cr_assert(args_is_code_literal(args), "Should detect code literal argument");
  cr_assert_str_eq(args_get_code_literal(args), "",
                   "Should handle empty code literal");

  args_free(&args);
}

// =========================
// Error Cases (Death Tests)
// =========================

Test(args, no_arguments_causes_exit, .exit_code = 1) {
  const char *argv[] = {"teeny"};
  const int argc = 1;
  MUTED_OUTPUT { parse_args(argc, argv); }
}

Test(args, too_many_arguments_causes_exit, .exit_code = 1) {
  const char *argv[] = {"teeny", "-c", "code", "extra"};
  const int argc = 4;

  MUTED_OUTPUT { parse_args(argc, argv); }
}

Test(args, wrong_flag_causes_exit, .exit_code = 1) {
  const char *argv[] = {"teeny", "-x", "code"};
  const int argc = 3;
  MUTED_OUTPUT { parse_args(argc, argv); }
}

Test(args, missing_code_after_flag_causes_exit, .exit_code = 1) {
  const char *argv[] = {"teeny", "-c"};
  const int argc = 2;

  // This should fail because -c requires an argument but we only have 2 args
  // total
  MUTED_OUTPUT { parse_args(argc, argv); }
}

// =========================
// Memory Management Tests
// =========================

Test(args, args_free_handles_null_pointer) {
  Args args = NULL;
  args_free(&args); // Should not crash
  cr_assert_null(args, "Args should remain null");
}

Test(args, args_free_sets_pointer_to_null) {
  const char *argv[] = {"teeny", "test.basic"};
  const int argc = 2;

  Args args = parse_args(argc, argv);
  cr_assert_not_null(args, "Args should not be null initially");

  args_free(&args);
  cr_assert_null(args, "Args should be null after freeing");
}
