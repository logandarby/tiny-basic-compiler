#include "../src/string_util.h"
#include <criterion/criterion.h>
#include <criterion/redirect.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// === TEST HELPERS ===

// Helper to create modifiable string copy
char *make_test_string(const char *literal) {
  if (literal == NULL)
    return NULL;
  char *copy = malloc(strlen(literal) + 1);
  cr_assert_not_null(copy, "Failed to allocate memory for test string");
  strcpy(copy, literal);
  return copy;
}

// Helper to verify string content and free
void verify_and_free(char *str, const char *expected, const char *test_name) {
  cr_assert_str_eq(str, expected, "Test failed: %s", test_name);
  free(str);
}

// Helper to verify string with embedded nulls
void verify_with_length_and_free(char *str, const char *expected,
                                 size_t expected_len, const char *test_name) {
  cr_assert_eq(memcmp(str, expected, expected_len), 0, "Test failed: %s",
               test_name);
  free(str);
}

// === BASIC FUNCTIONALITY TESTS ===

Test(basic_functionality, null_input) {
  bool result = string_clean_escape_sequences(NULL, NULL);
  cr_assert_eq(result, false, "Should return false for NULL input");
}

Test(basic_functionality, null_config) {
  char *str = make_test_string("Hello\\nWorld");
  bool result = string_clean_escape_sequences(str, NULL);
  cr_assert_eq(result, true, "Should handle NULL config (use default)");
  cr_assert_str_eq(str, "Hello\nWorld",
                   "Should process escapes with default config");
  free(str);
}

Test(basic_functionality, empty_string) {
  char *str = make_test_string("");
  bool result = string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  cr_assert_eq(result, true, "Should handle empty string");
  verify_and_free(str, "", "empty string");
}

Test(basic_functionality, no_escapes) {
  char *str = make_test_string("Hello World! 123 @#$%^&*()");
  bool result = string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  cr_assert_eq(result, true, "Should handle string with no escapes");
  verify_and_free(str, "Hello World! 123 @#$%^&*()", "no escapes");
}

Test(basic_functionality, single_character) {
  char *str = make_test_string("A");
  bool result = string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  cr_assert_eq(result, true, "Should handle single character");
  verify_and_free(str, "A", "single character");
}

// === DEFAULT ESCAPE MAPPING TESTS ===

Test(default_mappings, double_quote) {
  char *str = make_test_string("Say \\\"Hello\\\"");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "Say \"Hello\"", "double quote escape");
}

Test(default_mappings, single_quote) {
  char *str = make_test_string("It\\'s working");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "It's working", "single quote escape");
}

Test(default_mappings, backslash) {
  char *str = make_test_string("Path: C:\\\\Windows\\\\System32");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "Path: C:\\Windows\\System32", "backslash escape");
}

Test(default_mappings, newline) {
  char *str = make_test_string("Line 1\\nLine 2\\nLine 3");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "Line 1\nLine 2\nLine 3", "newline escape");
}

Test(default_mappings, tab) {
  char *str = make_test_string("Column1\\tColumn2\\tColumn3");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "Column1\tColumn2\tColumn3", "tab escape");
}

Test(default_mappings, carriage_return) {
  char *str = make_test_string("Line 1\\rOverwrite");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "Line 1\rOverwrite", "carriage return escape");
}

Test(default_mappings, backspace) {
  char *str = make_test_string("Hello\\bWorld");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "Hello\bWorld", "backspace escape");
}

Test(default_mappings, form_feed) {
  char *str = make_test_string("Page1\\fPage2");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "Page1\fPage2", "form feed escape");
}

Test(default_mappings, vertical_tab) {
  char *str = make_test_string("Line1\\vLine2");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "Line1\vLine2", "vertical tab escape");
}

Test(default_mappings, null_character) {
  char *str = make_test_string("Before\\0After");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  // Note: String will be truncated at null character
  verify_with_length_and_free(str, "Before\0After", 7, "null character escape");
}

// === COMPLEX ESCAPE COMBINATIONS ===

Test(complex_combinations, all_escapes_mixed) {
  char *str =
      make_test_string("\\\"Hello\\\\World\\\"\\n\\tNext\\rLine\\b\\f\\v");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "\"Hello\\World\"\n\tNext\rLine\b\f\v",
                  "all escapes mixed");
}

Test(complex_combinations, consecutive_escapes) {
  char *str = make_test_string("\\n\\t\\r\\\\\\\"");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "\n\t\r\\\"", "consecutive escapes");
}

Test(complex_combinations, repeated_same_escape) {
  char *str = make_test_string("\\n\\n\\n\\n");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "\n\n\n\n", "repeated same escape");
}

Test(complex_combinations, escape_at_start) {
  char *str = make_test_string("\\nStart with newline");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "\nStart with newline", "escape at start");
}

Test(complex_combinations, escape_at_end) {
  char *str = make_test_string("End with newline\\n");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "End with newline\n", "escape at end");
}

Test(complex_combinations, only_escapes) {
  char *str = make_test_string("\\\"\\n\\t\\\\");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "\"\n\t\\", "only escapes");
}

// === EDGE CASES ===

Test(edge_cases, lone_backslash_at_end) {
  char *str = make_test_string("Hello\\");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "Hello\\", "lone backslash at end");
}

Test(edge_cases, multiple_lone_backslashes) {
  char *str = make_test_string("\\Hello\\World\\");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "\\Hello\\World\\", "multiple lone backslashes");
}

Test(edge_cases, unknown_escape_preserved) {
  char *str = make_test_string("Unknown: \\x \\z \\123");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "Unknown: \\x \\z \\123", "unknown escapes preserved");
}

Test(edge_cases, backslash_before_null_terminator) {
  char str[] = {'H', 'e', 'l', 'l', 'o', '\\', '\0'};
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  cr_assert_str_eq(str, "Hello\\", "backslash before null terminator");
}

Test(edge_cases, very_long_string) {
  // Create a 1000 character string with mixed content
  char *str = malloc(1001);
  cr_assert_not_null(str);

  for (int i = 0; i < 100; i++) {
    sprintf(str + i * 10, "Hello\\n%03d", i);
  }
  str[1000] = '\0';

  bool result = string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  cr_assert_eq(result, true, "Should handle very long strings");

  // Verify some content (string should be shorter now)
  cr_assert(strlen(str) < 1000, "String should be shorter after processing");
  cr_assert(strstr(str, "Hello\n000") != NULL,
            "Should contain processed content");

  free(str);
}

Test(edge_cases, string_with_existing_newlines) {
  char *str = make_test_string("Line1\nLine2\\nLine3\nLine4");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "Line1\nLine2\nLine3\nLine4",
                  "mixed real and escaped newlines");
}

// === CUSTOM CONFIGURATION TESTS ===

Test(custom_config, quotes_only) {
  const EscapeMappingNode quotes_only[] = {{'"', '"'}, {'\'', '\''}};

  const EscapeConfig quotes_config = {
      .mappings = quotes_only, .count = 2, .preserve_unknown = false};

  char *str = make_test_string("\\\"Hello\\tWorld\\n\\\"");
  string_clean_escape_sequences(str, &quotes_config);
  verify_and_free(str, "\"HellotWorldn\"", "quotes only config");
}

Test(custom_config, preserve_unknown_false) {
  const EscapeMappingNode minimal[] = {{'n', '\n'}};

  const EscapeConfig minimal_config = {
      .mappings = minimal, .count = 1, .preserve_unknown = false};

  char *str = make_test_string("\\n\\t\\r\\x\\z");
  string_clean_escape_sequences(str, &minimal_config);
  verify_and_free(str, "\ntrxz", "preserve unknown false");
}

Test(custom_config, preserve_unknown_true) {
  const EscapeMappingNode minimal[] = {{'n', '\n'}};

  const EscapeConfig minimal_config = {
      .mappings = minimal, .count = 1, .preserve_unknown = true};

  char *str = make_test_string("\\n\\t\\r\\x\\z");
  string_clean_escape_sequences(str, &minimal_config);
  verify_and_free(str, "\n\\t\\r\\x\\z", "preserve unknown true");
}

Test(custom_config, empty_mapping_preserve_true) {
  const EscapeConfig empty_config = {
      .mappings = NULL, .count = 0, .preserve_unknown = true};

  char *str = make_test_string("\\n\\t\\r\\\"");
  string_clean_escape_sequences(str, &empty_config);
  verify_and_free(str, "\\n\\t\\r\\\"", "empty mapping preserve true");
}

Test(custom_config, empty_mapping_preserve_false) {
  const EscapeConfig empty_config = {
      .mappings = NULL, .count = 0, .preserve_unknown = false};

  char *str = make_test_string("\\n\\t\\r\\\"");
  string_clean_escape_sequences(str, &empty_config);
  verify_and_free(str, "ntr\"", "empty mapping preserve false");
}

Test(custom_config, custom_replacements) {
  const EscapeMappingNode custom[] = {
      {'X', 'Y'}, // \X -> Y
      {'1', '!'}, // \1 -> !
      {'a', '@'}  // \a -> @
  };

  const EscapeConfig custom_config = {
      .mappings = custom, .count = 3, .preserve_unknown = false};

  char *str = make_test_string("\\X\\1\\a\\n");
  string_clean_escape_sequences(str, &custom_config);
  verify_and_free(str, "Y!@n", "custom replacements");
}

// === MEMORY AND BOUNDARY TESTS ===

Test(memory_tests, stack_allocated_array) {
  char stack_str[] = "Stack: \\\"Hello\\tWorld\\n\\\"";
  bool result =
      string_clean_escape_sequences(stack_str, &DEFAULT_ESCAPE_CONFIG);
  cr_assert_eq(result, true, "Should work with stack allocated arrays");
  cr_assert_str_eq(stack_str, "Stack: \"Hello\tWorld\n\"",
                   "Stack array content correct");
}

Test(memory_tests, modification_in_place) {
  char *original = make_test_string("Before\\nAfter");
  char *ptr_before = original;

  string_clean_escape_sequences(original, &DEFAULT_ESCAPE_CONFIG);

  // Pointer should be the same (in-place modification)
  cr_assert_eq(original, ptr_before, "Should modify in-place");
  cr_assert_str_eq(original, "Before\nAfter", "Content should be correct");

  free(original);
}

Test(memory_tests, string_shortening) {
  char *str = make_test_string("\\n\\t\\r\\\\"); // 8 chars -> 4 chars
  size_t original_len = strlen(str);

  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);

  size_t new_len = strlen(str);
  cr_assert(new_len < original_len,
            "String should be shorter after processing");
  cr_assert_eq(new_len, 4, "Should be exactly 4 characters");

  free(str);
}

// === STRESS TESTS ===

Test(stress_tests, many_consecutive_backslashes) {
  // Test 100 consecutive escape sequences
  char *str = malloc(401); // 100 * 4 chars + 1
  cr_assert_not_null(str);

  for (int i = 0; i < 100; i++) {
    sprintf(str + i * 4, "\\n\\t");
  }
  str[400] = '\0';

  bool result = string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  cr_assert_eq(result, true, "Should handle many consecutive escapes");
  cr_assert_eq(strlen(str), 200, "Should result in 200 characters");

  free(str);
}

Test(stress_tests, alternating_pattern) {
  char *str = make_test_string("a\\nb\\tc\\rd\\\\e\\\"f\\'g");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "a\nb\tc\rd\\e\"f'g", "alternating pattern");
}

// === REGRESSION TESTS ===

Test(regression_tests, issue_null_char_truncation) {
  // Test that processing continues after null character in escape
  char input[] = {'\\', '0', 'A', 'B', 'C', '\0'};
  string_clean_escape_sequences(input, &DEFAULT_ESCAPE_CONFIG);

  // Should have null at position 0, but original string length info is lost
  cr_assert_eq(input[0], '\0', "Should have null character at start");
  // Note: The string is effectively truncated at the null character
}

Test(regression_tests, boundary_read_write_indices) {
  // Test case where read and write indices might get out of sync
  char *str = make_test_string("\\\\\\\\\\\\"); // Three escaped backslashes
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "\\\\\\", "boundary indices test");
}

// === UNICODE AND SPECIAL CHARACTER TESTS ===

Test(special_chars, unicode_preservation) {
  char *str = make_test_string("Unicode: café\\nñáéíóú");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);
  verify_and_free(str, "Unicode: café\nñáéíóú", "unicode preservation");
}

Test(special_chars, high_ascii) {
  char *str = make_test_string("High ASCII: \xFF\xFE\\n\xFD");
  string_clean_escape_sequences(str, &DEFAULT_ESCAPE_CONFIG);

  // Check specific bytes
  cr_assert_eq((unsigned char)str[12], 0xFF, "Should preserve high ASCII");
  cr_assert_eq((unsigned char)str[13], 0xFE, "Should preserve high ASCII");
  cr_assert_eq(str[14], '\n', "Should process escape");
  cr_assert_eq((unsigned char)str[15], 0xFD, "Should preserve high ASCII");

  free(str);
}