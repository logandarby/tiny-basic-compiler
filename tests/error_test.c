#include "../src/common/error_reporter.h"
#include "../src/common/file_reader.h"
#include "../src/debug/dz_debug.h"
#include "../src/frontend/lexer/lexer.h"
#include "token.h"
#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>

// Helper function to create a string from input and parse it
static TokenArray parse_string_with_errors(const char *input) {
  // Clear any existing errors before the test
  er_free();

  FileReader fr = filereader_init_from_string(input);
  TokenArray ta = lexer_parse(fr);
  filereader_destroy(&fr);
  return ta;
}

// Helper function to get error count
static uint32_t get_error_count(void) { return er_get_error_count(); }

// Helper function to get specific error
static CompilerError get_error_at(uint32_t index) {
  cr_assert_lt(index, get_error_count(),
               "Error index %" PRIu32 " out of bounds", index);
  return er_get_error_at(index);
}

// Helper function to check if an error contains specific text
static bool error_contains_text(const CompilerError *error, const char *text) {
  return error->message && strstr(error->message, text) != NULL;
}

// Helper function to find error with specific characteristics
static bool find_error_with_text_and_position(const char *text,
                                              uint32_t expected_line,
                                              uint32_t expected_col) {
  uint32_t count = get_error_count();
  for (uint32_t i = 0; i < count; i++) {
    CompilerError error = get_error_at(i);
    if (error_contains_text(&error, text) && error.line == expected_line &&
        error.col == expected_col) {
      return true;
    }
  }
  return false;
}

// =========================
// UNKNOWN CHARACTER ERROR TESTS
// =========================

Test(error_test, unknown_character_single) {
  TokenArray ta = parse_string_with_errors("@");

  // Should have exactly one error
  cr_assert_eq(get_error_count(), 1,
               "Should have exactly 1 error for unknown character");
  cr_assert(er_has_errors(), "Should have errors");

  // Check the error details
  CompilerError error = get_error_at(0);

  cr_assert_eq(error.type, ERROR_LEXICAL, "Should be a lexical error");
  cr_assert_eq(error.line, 1, "Error should be on line 1");
  cr_assert_eq(error.col, 1, "Error should be at column 1 (1-indexed)");
  cr_assert(error_contains_text(&error, "Invalid character"),
            "Error should mention invalid character");
  cr_assert(error_contains_text(&error, "@"),
            "Error should mention the @ character");

  // Should produce one TOKEN_UNKNOWN
  cr_assert_eq(token_array_length(ta), 1, "Should produce 1 token");
  cr_assert_eq(token_array_at(ta, 0).type, TOKEN_UNKNOWN,
               "Should be TOKEN_UNKNOWN");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, unknown_character_multiple) {
  TokenArray ta = parse_string_with_errors("@ # $ %");

  // Should have exactly 4 errors
  cr_assert_eq(get_error_count(), 4,
               "Should have exactly 4 errors for 4 unknown characters");

  // All errors should be lexical
  for (uint32_t i = 0; i < get_error_count(); i++) {
    CompilerError error = get_error_at(i);
    cr_assert_eq(error.type, ERROR_LEXICAL,
                 "Error %" PRIu32 " should be lexical", i);
    cr_assert_eq(error.line, 1, "Error %" PRIu32 " should be on line 1", i);
  }

  // Check specific positions (1-indexed)
  cr_assert(find_error_with_text_and_position("@", 1, 1),
            "Should find @ error at position 1:1");
  cr_assert(find_error_with_text_and_position("#", 1, 3),
            "Should find # error at position 1:3");
  cr_assert(find_error_with_text_and_position("$", 1, 5),
            "Should find $ error at position 1:5");
  cr_assert(find_error_with_text_and_position("%", 1, 7),
            "Should find % error at position 1:7");

  // Should produce 4 TOKEN_UNKNOWN
  cr_assert_eq(token_array_length(ta), 4, "Should produce 4 tokens");
  for (uint32_t i = 0; i < token_array_length(ta); i++) {
    cr_assert_eq(token_array_at(ta, i).type, TOKEN_UNKNOWN,
                 "Token %" PRIu32 " should be TOKEN_UNKNOWN", i);
  }

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, unknown_character_mixed_with_valid) {
  TokenArray ta = parse_string_with_errors("LET x @ 42");

  // Should have exactly 1 error
  cr_assert_eq(get_error_count(), 1, "Should have exactly 1 error");

  CompilerError error = get_error_at(0);
  cr_assert_eq(error.type, ERROR_LEXICAL, "Should be a lexical error");
  cr_assert_eq(error.line, 1, "Error should be on line 1");
  cr_assert_eq(error.col, 7, "Error should be at column 7 (position of @)");
  cr_assert(error_contains_text(&error, "@"),
            "Error should mention the @ character");

  // Should produce LET, x, TOKEN_UNKNOWN, 42
  cr_assert_eq(token_array_length(ta), 4, "Should produce 4 tokens");
  cr_assert_eq(token_array_at(ta, 0).type, TOKEN_LET,
               "First token should be LET");
  cr_assert_eq(token_array_at(ta, 1).type, TOKEN_IDENT,
               "Second token should be identifier");
  cr_assert_eq(token_array_at(ta, 2).type, TOKEN_UNKNOWN,
               "Third token should be TOKEN_UNKNOWN");
  cr_assert_eq(token_array_at(ta, 3).type, TOKEN_NUMBER,
               "Fourth token should be number");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, unknown_character_multiline) {
  TokenArray ta = parse_string_with_errors("line 1\nline @ 2\nline # 3");

  // Should have exactly 2 errors
  cr_assert_eq(get_error_count(), 2, "Should have exactly 2 errors");

  // Check @ error on line 2
  cr_assert(find_error_with_text_and_position("@", 2, 6),
            "Should find @ error at position 2:6");

  // Check # error on line 3
  cr_assert(find_error_with_text_and_position("#", 3, 6),
            "Should find # error at position 3:6");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, unknown_character_special_chars) {
  // Test various non-ASCII and special characters
  TokenArray ta = parse_string_with_errors("~`[]{}");

  // Should have exactly 6 errors (all these characters should be unknown)
  cr_assert_eq(get_error_count(), 6,
               "Should have 6 errors for special characters");

  // All should be lexical errors on line 1
  for (uint32_t i = 0; i < get_error_count(); i++) {
    CompilerError error = get_error_at(i);
    cr_assert_eq(error.type, ERROR_LEXICAL,
                 "Error %" PRIu32 " should be lexical", i);
    cr_assert_eq(error.line, 1, "Error %" PRIu32 " should be on line 1", i);
  }

  token_array_destroy(&ta);
  er_free();
}

// =========================
// UNTERMINATED STRING ERROR TESTS
// =========================

Test(error_test, unterminated_string_simple) {
  TokenArray ta = parse_string_with_errors("\"unterminated");

  // Should have exactly 1 error
  cr_assert_eq(get_error_count(), 1,
               "Should have exactly 1 error for unterminated string");
  cr_assert(er_has_errors(), "Should have errors");

  CompilerError error = get_error_at(0);

  cr_assert_eq(error.type, ERROR_LEXICAL, "Should be a lexical error");
  cr_assert_eq(error.line, 1, "Error should be on line 1");
  cr_assert_eq(error.col, 1,
               "Error should be at column 1 (position of opening quote)");
  cr_assert(error_contains_text(&error, "Unterminated string"),
            "Error should mention unterminated string");
  cr_assert(error_contains_text(&error, "unterminated"),
            "Error should contain the string content");

  // Should produce one TOKEN_UNKNOWN
  cr_assert_eq(token_array_length(ta), 1, "Should produce 1 token");
  cr_assert_eq(token_array_at(ta, 0).type, TOKEN_UNKNOWN,
               "Should be TOKEN_UNKNOWN");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, unterminated_string_empty) {
  TokenArray ta = parse_string_with_errors("\"\n");

  // Should have exactly 0 errors (single quote produces no error)
  cr_assert_eq(get_error_count(), 1,
               "Single quote alone should produce an error");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, unterminated_string_empty_2) {
  TokenArray ta = parse_string_with_errors("\"");

  // Should have exactly 0 errors (single quote produces no error)
  cr_assert_eq(get_error_count(), 1,
               "Single quote alone should produce an error");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, unterminated_string_empty_3) {
  TokenArray ta = parse_string_with_errors("     \"");

  // Should have exactly 0 errors (single quote produces no error)
  cr_assert_eq(get_error_count(), 1,
               "Single quote alone should produce an error");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, unterminated_string_with_valid_tokens) {
  TokenArray ta = parse_string_with_errors("LET x = \"hello world");

  // Should have exactly 1 error
  cr_assert_eq(get_error_count(), 1, "Should have exactly 1 error");

  CompilerError error = get_error_at(0);
  cr_assert_eq(error.type, ERROR_LEXICAL, "Should be a lexical error");
  cr_assert_eq(error.line, 1, "Error should be on line 1");
  cr_assert_eq(error.col, 9,
               "Error should be at column 9 (position of opening quote)");
  cr_assert(error_contains_text(&error, "Unterminated string"),
            "Error should mention unterminated string");
  cr_assert(error_contains_text(&error, "hello world"),
            "Error should contain the string content");

  // Should produce LET, x, =, TOKEN_UNKNOWN
  cr_assert_eq(token_array_length(ta), 4, "Should produce 4 tokens");
  cr_assert_eq(token_array_at(ta, 0).type, TOKEN_LET,
               "First token should be LET");
  cr_assert_eq(token_array_at(ta, 1).type, TOKEN_IDENT,
               "Second token should be identifier");
  cr_assert_eq(token_array_at(ta, 2).type, TOKEN_EQ,
               "Third token should be equals");
  cr_assert_eq(token_array_at(ta, 3).type, TOKEN_UNKNOWN,
               "Fourth token should be TOKEN_UNKNOWN");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, unterminated_string_multiline) {
  TokenArray ta = parse_string_with_errors("line 1\n\"unterminated on line 2");

  // Should have exactly 1 error
  cr_assert_eq(get_error_count(), 1, "Should have exactly 1 error");

  CompilerError error = get_error_at(0);
  cr_assert_eq(error.type, ERROR_LEXICAL, "Should be a lexical error");
  cr_assert_eq(error.line, 2, "Error should be on line 2");
  cr_assert_eq(error.col, 1,
               "Error should be at column 1 (position of opening quote)");
  cr_assert(error_contains_text(&error, "Unterminated string"),
            "Error should mention unterminated string");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, multiple_unterminated_strings) {
  TokenArray ta =
      parse_string_with_errors("\"first unterminated\n\"second unterminated");

  // Should have exactly 2 errors
  cr_assert_eq(get_error_count(), 2, "Should have exactly 2 errors");

  // Check first error
  cr_assert(find_error_with_text_and_position("first unterminated", 1, 1),
            "Should find first error at position 1:2");

  // Check second error
  cr_assert(find_error_with_text_and_position("second unterminated", 2, 1),
            "Should find second error at position 2:2");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, unterminated_string_single_quote) {
  TokenArray ta = parse_string_with_errors("'unterminated single quote");

  // Should have exactly 1 error
  cr_assert_eq(
      get_error_count(), 1,
      "Should have exactly 1 error for unterminated single quote string");

  CompilerError error = get_error_at(0);
  cr_assert_eq(error.type, ERROR_LEXICAL, "Should be a lexical error");
  cr_assert_eq(error.line, 1, "Error should be on line 1");
  cr_assert_eq(error.col, 1,
               "Error should be at column 1 (position of opening quote)");
  cr_assert(error_contains_text(&error, "Unterminated string"),
            "Error should mention unterminated string");
  cr_assert(error_contains_text(&error, "'"),
            "Error should mention single quote delimiter");

  token_array_destroy(&ta);
  er_free();
}

// =========================
// MIXED ERROR SCENARIOS
// =========================

Test(error_test, mixed_unknown_char_and_unterminated_string) {
  TokenArray ta = parse_string_with_errors("@ \"unterminated # string");

  // Should have exactly 2 errors: @, unterminated string (# is allowed in
  // strings)
  cr_assert_eq(get_error_count(), 2, "Should have exactly 2 errors");

  // Check @ error
  cr_assert(find_error_with_text_and_position("@", 1, 1),
            "Should find @ error at position 1:1");

  // Check unterminated string error
  bool found_unterminated = false;
  for (uint32_t i = 0; i < get_error_count(); i++) {
    CompilerError error = get_error_at(i);
    if (error_contains_text(&error, "Unterminated string")) {
      found_unterminated = true;
      cr_assert_eq(error.line, 1,
                   "Unterminated string error should be on line 1");
      cr_assert_eq(error.col, 3,
                   "Unterminated string error should be at column 3");
      break;
    }
  }
  cr_assert(found_unterminated, "Should find unterminated string error");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, errors_with_correct_file_position_complex) {
  TokenArray ta =
      parse_string_with_errors("  LET    x    @    42   \"unterminated   #   ");

  // Should have 2 errors: @, unterminated string
  cr_assert_eq(get_error_count(), 2, "Should have exactly 2 errors");

  // @ should be at position where it appears (count spaces)
  // "  LET    x    @" - @ is at position 15 (1-indexed)
  cr_assert(find_error_with_text_and_position("@", 1, 15),
            "Should find @ error at correct position");

  // Unterminated string starts after "42   "
  bool found_unterminated = false;
  for (uint32_t i = 0; i < get_error_count(); i++) {
    CompilerError error = get_error_at(i);
    if (error_contains_text(&error, "Unterminated string")) {
      found_unterminated = true;
      cr_assert_eq(error.line, 1, "Unterminated string should be on line 1");
      break;
    }
  }
  cr_assert(found_unterminated, "Should find unterminated string error");

  token_array_destroy(&ta);
  er_free();
}

// =========================
// EDGE CASES
// =========================

Test(error_test, no_errors_with_valid_input) {
  TokenArray ta = parse_string_with_errors("LET x = 42 + \"hello world\"");

  // Should have no errors
  cr_assert_eq(get_error_count(), 0, "Should have no errors with valid input");
  cr_assert(!er_has_errors(), "Should not have errors");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, error_positions_across_multiple_lines) {
  TokenArray ta =
      parse_string_with_errors("line 1 ok\n  @ on line 2\n    \"unterminated "
                               "on line 3\n      # on line 4");

  // Should have 3 errors
  cr_assert_eq(get_error_count(), 3, "Should have exactly 3 errors");

  // Check @ error on line 2, column 3
  cr_assert(find_error_with_text_and_position("@", 2, 3),
            "Should find @ error at position 2:3");

  // Check unterminated string error on line 3, column 5
  bool found_unterminated = false;
  for (uint32_t i = 0; i < get_error_count(); i++) {
    CompilerError error = get_error_at(i);
    if (error_contains_text(&error, "Unterminated string")) {
      found_unterminated = true;
      cr_assert_eq(error.line, 3, "Unterminated string should be on line 3");
      cr_assert_eq(error.col, 5, "Unterminated string should be at column 5");
      break;
    }
  }
  cr_assert(found_unterminated, "Should find unterminated string error");

  // Check # error on line 4, column 7
  cr_assert(find_error_with_text_and_position("#", 4, 7),
            "Should find # error at position 4:7");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, empty_input_no_errors) {
  TokenArray ta = parse_string_with_errors("");

  // Should have no errors
  cr_assert_eq(get_error_count(), 0, "Should have no errors with empty input");
  cr_assert(!er_has_errors(), "Should not have errors");
  cr_assert_eq(token_array_length(ta), 0, "Should produce no tokens");

  token_array_destroy(&ta);
  er_free();
}

Test(error_test, whitespace_only_no_errors) {
  TokenArray ta = parse_string_with_errors("   \t\n\n  \t ");

  // Should have no errors
  cr_assert_eq(get_error_count(), 0,
               "Should have no errors with whitespace only");
  cr_assert(!er_has_errors(), "Should not have errors");
  cr_assert_eq(token_array_length(ta), 0, "Should produce no tokens");

  token_array_destroy(&ta);
  er_free();
}
