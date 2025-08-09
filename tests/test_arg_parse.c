#include "../src/common/arg_parse.h"
#include <criterion/criterion.h>
#include <criterion/new/assert.h>

// Test helper to create argv array
static const char **create_argv(int argc, ...) {
  va_list args;
  const char **argv = malloc(argc * sizeof(char *));

  va_start(args, argc);
  for (int i = 0; i < argc; i++) {
    argv[i] = va_arg(args, const char *);
  }
  va_end(args);

  return argv;
}

// Test helper to free argv array
static void free_argv(const char **argv) { free(argv); }

// === Basic Flag Tests ===

Test(arg_parse, test_simple_flag) {
  static const FlagSpec flags[] = {
      FLAG('v', "verbose", "Enable verbose output"),
      FLAG('h', "help", "Show help message")};
  static const ArgSpec args[] = {};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 2,
                                  .args = args,
                                  .arg_count = 0};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test short flag
  const char **argv1 = create_argv(2, "test", "-v");
  ParseResult *result1 = argparse_parse(parser, 2, argv1);
  cr_assert_not_null(result1, "Parse result is null");
  cr_assert(argparse_is_success(result1), "Parse should succeed");
  cr_assert(argparse_has_flag(result1, "v"), "Should have -v flag");
  cr_assert(!argparse_has_flag(result1, "h"), "Should not have -h flag");

  // Test long flag
  const char **argv2 = create_argv(2, "test", "--verbose");
  ParseResult *result2 = argparse_parse(parser, 2, argv2);
  cr_assert_not_null(result2, "Parse result is null");
  cr_assert(argparse_is_success(result2), "Parse should succeed");
  cr_assert(argparse_has_flag(result2, "verbose"),
            "Should have --verbose flag");
  cr_assert(argparse_has_flag(result2, "v"), "Should also match by short name");

  // Cleanup
  free_argv(argv1);
  free_argv(argv2);
  argparse_free_result(result1);
  argparse_free_result(result2);
  argparse_free_parser(parser);
}

Test(arg_parse, test_flag_with_value) {
  static const FlagSpec flags[] = {
      FLAG_WITH_VALUE('o', "output", "Output file"),
      FLAG('v', "verbose", "Enable verbose output")};
  static const ArgSpec args[] = {};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 2,
                                  .args = args,
                                  .arg_count = 0};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test short flag with value
  const char **argv1 = create_argv(3, "test", "-o", "output.txt");
  ParseResult *result1 = argparse_parse(parser, 3, argv1);
  cr_assert_not_null(result1, "Parse result is null");
  cr_assert(argparse_is_success(result1), "Parse should succeed");
  cr_assert(argparse_has_flag(result1, "o"), "Should have -o flag");
  cr_assert_str_eq(argparse_get_flag_value(result1, "o"), "output.txt",
                   "Flag value should match");

  // Test long flag with value
  const char **argv2 = create_argv(3, "test", "--output", "file.out");
  ParseResult *result2 = argparse_parse(parser, 3, argv2);
  cr_assert_not_null(result2, "Parse result is null");
  cr_assert(argparse_is_success(result2), "Parse should succeed");
  cr_assert(argparse_has_flag(result2, "output"), "Should have --output flag");
  cr_assert_str_eq(argparse_get_flag_value(result2, "output"), "file.out",
                   "Flag value should match");

  // Cleanup
  free_argv(argv1);
  free_argv(argv2);
  argparse_free_result(result1);
  argparse_free_result(result2);
  argparse_free_parser(parser);
}

Test(arg_parse, test_required_flag_with_value) {
  static const FlagSpec flags[] = {
      REQUIRED_FLAG_WITH_VALUE('r', "required", "Required flag"),
      FLAG('v', "verbose", "Enable verbose output")};
  static const ArgSpec args[] = {};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 2,
                                  .args = args,
                                  .arg_count = 0};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test with required flag present
  const char **argv1 = create_argv(3, "test", "-r", "value");
  ParseResult *result1 = argparse_parse(parser, 3, argv1);
  cr_assert_not_null(result1, "Parse result is null");
  cr_assert(argparse_is_success(result1), "Parse should succeed");
  cr_assert(argparse_has_flag(result1, "r"), "Should have -r flag");
  cr_assert_str_eq(argparse_get_flag_value(result1, "r"), "value",
                   "Flag value should match");

  // Test without required flag (should fail)
  const char **argv2 = create_argv(2, "test", "-v");
  ParseResult *result2 = argparse_parse(parser, 2, argv2);
  cr_assert_not_null(result2, "Parse result is null");
  cr_assert(!argparse_is_success(result2),
            "Parse should fail without required flag");
  cr_assert_not_null(argparse_get_error(result2), "Should have error message");

  // Cleanup
  free_argv(argv1);
  free_argv(argv2);
  argparse_free_result(result1);
  argparse_free_result(result2);
  argparse_free_parser(parser);
}

// === Argument Tests ===

Test(arg_parse, test_required_arguments) {
  static const FlagSpec flags[] = {};
  static const ArgSpec args[] = {
      REQUIRED_ARG("input_file", "Input file to process"),
      OPTIONAL_ARG("output_file", "Output file")};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 0,
                                  .args = args,
                                  .arg_count = 2};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test with required argument
  const char **argv1 = create_argv(2, "test", "input.txt");
  ParseResult *result1 = argparse_parse(parser, 2, argv1);
  cr_assert_not_null(result1, "Parse result is null");
  cr_assert(argparse_is_success(result1), "Parse should succeed");
  cr_assert_str_eq(argparse_get_arg_value(result1, "input_file"), "input.txt",
                   "Required arg should match");
  cr_assert_null(argparse_get_arg_value(result1, "output_file"),
                 "Optional arg should be null");

  // Test with both arguments
  const char **argv2 = create_argv(3, "test", "input.txt", "output.txt");
  ParseResult *result2 = argparse_parse(parser, 3, argv2);
  cr_assert_not_null(result2, "Parse result is null");
  cr_assert(argparse_is_success(result2), "Parse should succeed");
  cr_assert_str_eq(argparse_get_arg_value(result2, "input_file"), "input.txt",
                   "Required arg should match");
  cr_assert_str_eq(argparse_get_arg_value(result2, "output_file"), "output.txt",
                   "Optional arg should match");

  // Test without required argument (should fail)
  const char **argv3 = create_argv(1, "test");
  ParseResult *result3 = argparse_parse(parser, 1, argv3);
  cr_assert_not_null(result3, "Parse result is null");
  cr_assert(!argparse_is_success(result3),
            "Parse should fail without required arg");
  cr_assert_not_null(argparse_get_error(result3), "Should have error message");

  // Cleanup
  free_argv(argv1);
  free_argv(argv2);
  free_argv(argv3);
  argparse_free_result(result1);
  argparse_free_result(result2);
  argparse_free_result(result3);
  argparse_free_parser(parser);
}

// === Compound Short Flag Tests ===

Test(arg_parse, test_compound_flags_without_values) {
  static const FlagSpec flags[] = {
      FLAG('v', "verbose", "Enable verbose output"),
      FLAG('c', "compile", "Compile mode"),
      FLAG('h', "help", "Show help message")};
  static const ArgSpec args[] = {};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 3,
                                  .args = args,
                                  .arg_count = 0};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test compound flags -vc
  const char **argv1 = create_argv(2, "test", "-vc");
  ParseResult *result1 = argparse_parse(parser, 2, argv1);
  cr_assert_not_null(result1, "Parse result is null");
  cr_assert(argparse_is_success(result1), "Parse should succeed");
  cr_assert(argparse_has_flag(result1, "v"), "Should have -v flag");
  cr_assert(argparse_has_flag(result1, "c"), "Should have -c flag");
  cr_assert(!argparse_has_flag(result1, "h"), "Should not have -h flag");

  // Test compound flags -vch
  const char **argv2 = create_argv(2, "test", "-vch");
  ParseResult *result2 = argparse_parse(parser, 2, argv2);
  cr_assert_not_null(result2, "Parse result is null");
  cr_assert(argparse_is_success(result2), "Parse should succeed");
  cr_assert(argparse_has_flag(result2, "v"), "Should have -v flag");
  cr_assert(argparse_has_flag(result2, "c"), "Should have -c flag");
  cr_assert(argparse_has_flag(result2, "h"), "Should have -h flag");

  // Cleanup
  free_argv(argv1);
  free_argv(argv2);
  argparse_free_result(result1);
  argparse_free_result(result2);
  argparse_free_parser(parser);
}

Test(arg_parse, test_compound_flags_with_value_at_end) {
  static const FlagSpec flags[] = {
      FLAG('v', "verbose", "Enable verbose output"),
      FLAG('c', "compile", "Compile mode"),
      FLAG_WITH_VALUE('o', "output", "Output file")};
  static const ArgSpec args[] = {};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 3,
                                  .args = args,
                                  .arg_count = 0};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test compound flags with value at end: -vco output.txt
  const char **argv = create_argv(3, "test", "-vco", "output.txt");
  ParseResult *result = argparse_parse(parser, 3, argv);
  cr_assert_not_null(result, "Parse result is null");
  cr_assert(argparse_is_success(result), "Parse should succeed");
  cr_assert(argparse_has_flag(result, "v"), "Should have -v flag");
  cr_assert(argparse_has_flag(result, "c"), "Should have -c flag");
  cr_assert(argparse_has_flag(result, "o"), "Should have -o flag");
  cr_assert_str_eq(argparse_get_flag_value(result, "o"), "output.txt",
                   "Flag value should match");

  // Cleanup
  free_argv(argv);
  argparse_free_result(result);
  argparse_free_parser(parser);
}

// === Error Handling Tests ===

Test(arg_parse, test_unknown_flag_error) {
  static const FlagSpec flags[] = {
      FLAG('v', "verbose", "Enable verbose output")};
  static const ArgSpec args[] = {};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 1,
                                  .args = args,
                                  .arg_count = 0};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test unknown short flag
  const char **argv1 = create_argv(2, "test", "-x");
  ParseResult *result1 = argparse_parse(parser, 2, argv1);
  cr_assert_not_null(result1, "Parse result is null");
  cr_assert(!argparse_is_success(result1),
            "Parse should fail with unknown flag");
  cr_assert_not_null(argparse_get_error(result1), "Should have error message");

  // Test unknown long flag
  const char **argv2 = create_argv(2, "test", "--unknown");
  ParseResult *result2 = argparse_parse(parser, 2, argv2);
  cr_assert_not_null(result2, "Parse result is null");
  cr_assert(!argparse_is_success(result2),
            "Parse should fail with unknown flag");
  cr_assert_not_null(argparse_get_error(result2), "Should have error message");

  // Cleanup
  free_argv(argv1);
  free_argv(argv2);
  argparse_free_result(result1);
  argparse_free_result(result2);
  argparse_free_parser(parser);
}

Test(arg_parse, test_flag_requires_value_error) {
  static const FlagSpec flags[] = {
      FLAG_WITH_VALUE('o', "output", "Output file")};
  static const ArgSpec args[] = {};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 1,
                                  .args = args,
                                  .arg_count = 0};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test flag without value
  const char **argv1 = create_argv(2, "test", "-o");
  ParseResult *result1 = argparse_parse(parser, 2, argv1);
  cr_assert_not_null(result1, "Parse result is null");
  cr_assert(!argparse_is_success(result1),
            "Parse should fail when flag requires value");
  cr_assert_not_null(argparse_get_error(result1), "Should have error message");

  // Test flag with next argument being another flag
  const char **argv2 = create_argv(3, "test", "-o", "-v");
  ParseResult *result2 = argparse_parse(parser, 3, argv2);
  cr_assert_not_null(result2, "Parse result is null");
  cr_assert(!argparse_is_success(result2),
            "Parse should fail when flag requires value");
  cr_assert_not_null(argparse_get_error(result2), "Should have error message");

  // Cleanup
  free_argv(argv1);
  free_argv(argv2);
  argparse_free_result(result1);
  argparse_free_result(result2);
  argparse_free_parser(parser);
}

Test(arg_parse, test_compound_flag_value_not_last_error) {
  static const FlagSpec flags[] = {
      FLAG('v', "verbose", "Enable verbose output"),
      FLAG_WITH_VALUE('o', "output", "Output file"),
      FLAG('c', "compile", "Compile mode")};
  static const ArgSpec args[] = {};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 3,
                                  .args = args,
                                  .arg_count = 0};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test compound flags with value-requiring flag not at end: -voc
  const char **argv = create_argv(2, "test", "-voc");
  ParseResult *result = argparse_parse(parser, 2, argv);
  cr_assert_not_null(result, "Parse result is null");
  cr_assert(!argparse_is_success(result),
            "Parse should fail when value flag is not last");
  cr_assert_not_null(argparse_get_error(result), "Should have error message");

  // Cleanup
  free_argv(argv);
  argparse_free_result(result);
  argparse_free_parser(parser);
}

// === Edge Cases and Boundary Tests ===

Test(arg_parse, test_empty_arguments) {
  static const FlagSpec flags[] = {};
  static const ArgSpec args[] = {};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 0,
                                  .args = args,
                                  .arg_count = 0};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test with just program name
  const char **argv = create_argv(1, "test");
  ParseResult *result = argparse_parse(parser, 1, argv);
  cr_assert_not_null(result, "Parse result is null");
  cr_assert(argparse_is_success(result), "Parse should succeed with no args");

  // Cleanup
  free_argv(argv);
  argparse_free_result(result);
  argparse_free_parser(parser);
}

Test(arg_parse, test_remaining_arguments) {
  static const FlagSpec flags[] = {
      FLAG('v', "verbose", "Enable verbose output")};
  static const ArgSpec args[] = {REQUIRED_ARG("input", "Input file")};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 1,
                                  .args = args,
                                  .arg_count = 1};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test with extra arguments
  const char **argv =
      create_argv(5, "test", "-v", "input.txt", "extra1", "extra2");
  ParseResult *result = argparse_parse(parser, 5, argv);
  cr_assert_not_null(result, "Parse result is null");
  cr_assert(argparse_is_success(result), "Parse should succeed");
  cr_assert(argparse_has_flag(result, "v"), "Should have -v flag");
  cr_assert_str_eq(argparse_get_arg_value(result, "input"), "input.txt",
                   "Input arg should match");

  int remaining_count;
  char **remaining = argparse_get_remaining_args(result, &remaining_count);
  cr_assert_eq(remaining_count, 2, "Should have 2 remaining args");
  cr_assert_str_eq(remaining[0], "extra1", "First remaining arg should match");
  cr_assert_str_eq(remaining[1], "extra2", "Second remaining arg should match");

  // Cleanup
  free_argv(argv);
  argparse_free_result(result);
  argparse_free_parser(parser);
}

Test(arg_parse, test_end_of_options_marker) {
  static const FlagSpec flags[] = {
      FLAG('v', "verbose", "Enable verbose output")};
  static const ArgSpec args[] = {REQUIRED_ARG("input", "Input file")};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 1,
                                  .args = args,
                                  .arg_count = 1};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test with -- end of options marker
  const char **argv =
      create_argv(5, "test", "-v", "--", "-not-a-flag", "input.txt");
  ParseResult *result = argparse_parse(parser, 5, argv);
  cr_assert_not_null(result, "Parse result is null");
  cr_assert(argparse_is_success(result), "Parse should succeed");
  cr_assert(argparse_has_flag(result, "v"), "Should have -v flag");
  cr_assert_str_eq(argparse_get_arg_value(result, "input"), "-not-a-flag",
                   "Input should be treated as literal");

  int remaining_count;
  char **remaining = argparse_get_remaining_args(result, &remaining_count);
  cr_assert_eq(remaining_count, 1, "Should have 1 remaining arg");
  cr_assert_str_eq(remaining[0], "input.txt", "Remaining arg should match");

  // Cleanup
  free_argv(argv);
  argparse_free_result(result);
  argparse_free_parser(parser);
}

Test(arg_parse, test_null_input_handling) {
  // Test null parser spec
  ArgParser *parser1 = argparse_create(NULL);
  cr_assert_null(parser1, "Parser should be null for null spec");

  // Create valid parser for other null tests
  static const FlagSpec flags[] = {
      FLAG('v', "verbose", "Enable verbose output")};
  static const ArgSpec args[] = {};
  static const ParserSpec spec = {.program_name = "test",
                                  .description = "Test program",
                                  .flags = flags,
                                  .flag_count = 1,
                                  .args = args,
                                  .arg_count = 0};

  ArgParser *parser = argparse_create(&spec);
  cr_assert_not_null(parser, "Parser creation failed");

  // Test null parser in parse
  ParseResult *result1 = argparse_parse(NULL, 1, NULL);
  cr_assert_null(result1, "Parse result should be null for null parser");

  // Test query functions with null results
  cr_assert(!argparse_has_flag(NULL, "v"),
            "Should return false for null result");
  cr_assert_null(argparse_get_flag_value(NULL, "v"),
                 "Should return null for null result");
  cr_assert_null(argparse_get_arg_value(NULL, "input"),
                 "Should return null for null result");

  int count;
  cr_assert_null(argparse_get_remaining_args(NULL, &count),
                 "Should return null for null result");
  cr_assert_eq(count, 0, "Count should be 0 for null result");

  cr_assert(!argparse_is_success(NULL), "Should return false for null result");
  cr_assert_null(argparse_get_error(NULL),
                 "Should return null for null result");

  // Cleanup
  argparse_free_parser(parser);
}