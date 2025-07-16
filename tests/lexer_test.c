#include "../src/lexer.h"
#include <criterion/criterion.h>
#include <criterion/parameterized.h>
#include <stdio.h>
#include <string.h>

// Helper function to create a string from input and parse it
static TokenArray parse_string(const char *input) {
  FileReader fr = filereader_init_from_string(input);
  TokenArray ta = lexer_parse(fr);
  filereader_destroy(&fr);
  return ta;
}

// Helper function to check if token array contains expected tokens
static void assert_tokens_equal(TokenArray ta, enum TOKEN *expected,
                                size_t expected_count) {
  cr_assert_eq(token_array_length(ta), expected_count,
               "Expected %zu tokens, got %zu", expected_count,
               token_array_length(ta));

  for (size_t i = 0; i < expected_count; i++) {
    enum TOKEN actual = token_array_at(ta, i);
    cr_assert_eq(actual, expected[i], "Token %zu: expected %d, got %d", i,
                 expected[i], actual);
  }
}

// =========================
// SINGLE CHARACTER OPERATORS
// =========================

Test(lexer, single_char_arithmetic_operators) {
  TokenArray ta = parse_string("+ - * /");

  enum TOKEN expected[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULT, TOKEN_DIV};
  assert_tokens_equal(ta, expected, 4);

  token_array_destroy(&ta);
}

Test(lexer, single_char_comparison_operators) {
  TokenArray ta = parse_string("> < =");

  enum TOKEN expected[] = {TOKEN_GT, TOKEN_LT, TOKEN_EQ};
  assert_tokens_equal(ta, expected, 3);

  token_array_destroy(&ta);
}

Test(lexer, single_char_logic_operators) {
  TokenArray ta = parse_string("!");

  enum TOKEN expected[] = {TOKEN_NOT};
  assert_tokens_equal(ta, expected, 1);

  token_array_destroy(&ta);
}

// =========================
// DOUBLE CHARACTER OPERATORS
// =========================

Test(lexer, double_char_comparison_operators) {
  TokenArray ta = parse_string(">= <= == !=");

  enum TOKEN expected[] = {TOKEN_GTE, TOKEN_LTE, TOKEN_EQEQ, TOKEN_NOTEQ};
  assert_tokens_equal(ta, expected, 4);

  token_array_destroy(&ta);
}

Test(lexer, double_char_logic_operators) {
  TokenArray ta = parse_string("&& ||");

  enum TOKEN expected[] = {TOKEN_AND, TOKEN_OR};
  assert_tokens_equal(ta, expected, 2);

  token_array_destroy(&ta);
}

// =========================
// MIXED OPERATOR COMBINATIONS
// =========================

Test(lexer, mixed_single_and_double_operators) {
  TokenArray ta = parse_string("+ >= - <= * == / !=");

  enum TOKEN expected[] = {TOKEN_PLUS, TOKEN_GTE,  TOKEN_MINUS, TOKEN_LTE,
                           TOKEN_MULT, TOKEN_EQEQ, TOKEN_DIV,   TOKEN_NOTEQ};
  assert_tokens_equal(ta, expected, 8);

  token_array_destroy(&ta);
}

Test(lexer, all_operators_combined) {
  TokenArray ta = parse_string("+ - * / > < >= <= = == != ! && ||");

  enum TOKEN expected[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULT,  TOKEN_DIV,
                           TOKEN_GT,   TOKEN_LT,    TOKEN_GTE,   TOKEN_LTE,
                           TOKEN_EQ,   TOKEN_EQEQ,  TOKEN_NOTEQ, TOKEN_NOT,
                           TOKEN_AND,  TOKEN_OR};
  assert_tokens_equal(ta, expected, 14);

  token_array_destroy(&ta);
}

// =========================
// THREE CHARACTER OPERATOR REJECTION
// =========================

Test(lexer, three_char_operators_rejected) {
  // Test various 3-character operator combinations that should be rejected
  const char *three_char_ops[] = {
      "===", // Three equals
      "!==", // JavaScript-style
      ">>>", // Three greater than
      "<<<", // Three less than
      "&&&", // Three ampersands
      "|||", // Three pipes
      "+++", // Three plus
      "---", // Three minus
      "***", // Three multiply
      "///", // Three divide
  };

  size_t num_tests = sizeof(three_char_ops) / sizeof(three_char_ops[0]);

  for (size_t i = 0; i < num_tests; i++) {
    TokenArray ta = parse_string(three_char_ops[i]);

    // Should contain exactly one TOKEN_UNKNOWN
    cr_assert_eq(token_array_length(ta), 1,
                 "3-char operator '%s' should produce exactly 1 token",
                 three_char_ops[i]);
    cr_assert_eq(token_array_at(ta, 0), TOKEN_UNKNOWN,
                 "3-char operator '%s' should be TOKEN_UNKNOWN",
                 three_char_ops[i]);

    token_array_destroy(&ta);
  }
}

Test(lexer, four_char_operators_rejected) {
  TokenArray ta = parse_string("===="
                               " !==="
                               " >>>>");

  // Should produce 3 TOKEN_UNKNOWN tokens
  enum TOKEN expected[] = {TOKEN_UNKNOWN, TOKEN_UNKNOWN, TOKEN_UNKNOWN};
  assert_tokens_equal(ta, expected, 3);

  token_array_destroy(&ta);
}

// =========================
// WHITESPACE SEPARATION TESTS
// =========================

Test(lexer, operators_with_various_whitespace) {
  TokenArray ta = parse_string("  +   -\t\t*\n\n/  ");

  enum TOKEN expected[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULT, TOKEN_DIV};
  assert_tokens_equal(ta, expected, 4);

  token_array_destroy(&ta);
}

Test(lexer, operators_with_newlines) {
  TokenArray ta = parse_string("+\n-\n*\n/\n>=\n<=\n==\n!=");

  enum TOKEN expected[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULT, TOKEN_DIV,
                           TOKEN_GTE,  TOKEN_LTE,   TOKEN_EQEQ, TOKEN_NOTEQ};
  assert_tokens_equal(ta, expected, 8);

  token_array_destroy(&ta);
}

Test(lexer, operators_with_tabs) {
  TokenArray ta = parse_string("+\t-\t*\t/\t>=\t<=");

  enum TOKEN expected[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULT,
                           TOKEN_DIV,  TOKEN_GTE,   TOKEN_LTE};
  assert_tokens_equal(ta, expected, 6);

  token_array_destroy(&ta);
}

// =========================
// EDGE CASES
// =========================

Test(lexer, single_operator_only) {
  TokenArray ta = parse_string("+");

  enum TOKEN expected[] = {TOKEN_PLUS};
  assert_tokens_equal(ta, expected, 1);

  token_array_destroy(&ta);
}

Test(lexer, double_operator_only) {
  TokenArray ta = parse_string(">=");

  enum TOKEN expected[] = {TOKEN_GTE};
  assert_tokens_equal(ta, expected, 1);

  token_array_destroy(&ta);
}

Test(lexer, empty_input) {
  TokenArray ta = parse_string("");

  cr_assert_eq(token_array_length(ta), 0,
               "Empty input should produce no tokens");

  token_array_destroy(&ta);
}

Test(lexer, whitespace_only) {
  TokenArray ta = parse_string("   \t\t\n\n  ");

  cr_assert_eq(token_array_length(ta), 0,
               "Whitespace-only input should produce no tokens");

  token_array_destroy(&ta);
}

// =========================
// OPERATOR BOUNDARY TESTS
// =========================

Test(lexer, adjacent_different_operators) {
  // These should be treated as separate words when separated by whitespace
  TokenArray ta = parse_string("+ - * / > < = !");

  enum TOKEN expected[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULT, TOKEN_DIV,
                           TOKEN_GT,   TOKEN_LT,    TOKEN_EQ,   TOKEN_NOT};
  assert_tokens_equal(ta, expected, 8);

  token_array_destroy(&ta);
}

Test(lexer, double_operators_separated) {
  TokenArray ta = parse_string(">= <= == != && ||");

  enum TOKEN expected[] = {TOKEN_GTE,   TOKEN_LTE, TOKEN_EQEQ,
                           TOKEN_NOTEQ, TOKEN_AND, TOKEN_OR};
  assert_tokens_equal(ta, expected, 6);

  token_array_destroy(&ta);
}

// =========================
// COMPREHENSIVE COMBINATION TESTS
// =========================

Test(lexer, realistic_expression_operators) {
  // Simulate realistic expressions with operators
  TokenArray ta = parse_string("+ - >= <= == != && ||");

  enum TOKEN expected[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_GTE, TOKEN_LTE,
                           TOKEN_EQEQ, TOKEN_NOTEQ, TOKEN_AND, TOKEN_OR};
  assert_tokens_equal(ta, expected, 8);

  token_array_destroy(&ta);
}

Test(lexer, all_arithmetic_operators) {
  TokenArray ta = parse_string("+ - * /");

  enum TOKEN expected[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULT, TOKEN_DIV};
  assert_tokens_equal(ta, expected, 4);

  token_array_destroy(&ta);
}

Test(lexer, all_comparison_operators) {
  TokenArray ta = parse_string("> < >= <= = == !=");

  enum TOKEN expected[] = {TOKEN_GT, TOKEN_LT,   TOKEN_GTE,  TOKEN_LTE,
                           TOKEN_EQ, TOKEN_EQEQ, TOKEN_NOTEQ};
  assert_tokens_equal(ta, expected, 7);

  token_array_destroy(&ta);
}

Test(lexer, all_logical_operators) {
  TokenArray ta = parse_string("! && ||");

  enum TOKEN expected[] = {TOKEN_NOT, TOKEN_AND, TOKEN_OR};
  assert_tokens_equal(ta, expected, 3);

  token_array_destroy(&ta);
}

// =========================
// STRESS TESTS
// =========================

Test(lexer, many_single_operators) {
  TokenArray ta = parse_string("+ + + + + + + + + +");

  enum TOKEN expected[10];
  for (int i = 0; i < 10; i++) {
    expected[i] = TOKEN_PLUS;
  }
  assert_tokens_equal(ta, expected, 10);

  token_array_destroy(&ta);
}

Test(lexer, many_double_operators) {
  TokenArray ta = parse_string(">= >= >= >= >=");

  enum TOKEN expected[5];
  for (int i = 0; i < 5; i++) {
    expected[i] = TOKEN_GTE;
  }
  assert_tokens_equal(ta, expected, 5);

  token_array_destroy(&ta);
}

Test(lexer, alternating_operators) {
  TokenArray ta = parse_string("+ - + - + -");

  enum TOKEN expected[] = {TOKEN_PLUS,  TOKEN_MINUS, TOKEN_PLUS,
                           TOKEN_MINUS, TOKEN_PLUS,  TOKEN_MINUS};
  assert_tokens_equal(ta, expected, 6);

  token_array_destroy(&ta);
}

// =========================
// MIXED WITH OTHER TOKENS TESTS
// =========================

Test(lexer, operators_with_keywords) {
  TokenArray ta = parse_string("IF + THEN - ELSE *");

  enum TOKEN expected[] = {TOKEN_IF,    TOKEN_PLUS, TOKEN_THEN,
                           TOKEN_MINUS, TOKEN_ELSE, TOKEN_MULT};
  assert_tokens_equal(ta, expected, 6);

  token_array_destroy(&ta);
}

Test(lexer, operators_with_numbers) {
  TokenArray ta = parse_string("42 + 123 - 456");

  enum TOKEN expected[] = {TOKEN_NUMBER, TOKEN_PLUS, TOKEN_NUMBER, TOKEN_MINUS,
                           TOKEN_NUMBER};
  assert_tokens_equal(ta, expected, 5);

  token_array_destroy(&ta);
}

Test(lexer, operators_with_identifiers) {
  TokenArray ta = parse_string("x + y - z");

  enum TOKEN expected[] = {TOKEN_IDENT, TOKEN_PLUS, TOKEN_IDENT, TOKEN_MINUS,
                           TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 5);

  token_array_destroy(&ta);
}

// =========================
// ERROR HANDLING TESTS
// =========================

Test(lexer, null_input_handling) {
  FileReader fr = filereader_init_from_string(NULL);
  cr_assert_null(fr, "Should handle NULL input gracefully");
}

Test(lexer, token_array_operations) {
  TokenArray ta = token_array_init();

  cr_assert_not_null(ta, "TokenArray should initialize successfully");
  cr_assert_eq(token_array_length(ta), 0, "New TokenArray should be empty");
  cr_assert(token_array_is_empty(ta), "New TokenArray should report as empty");

  token_array_push(ta, TOKEN_PLUS);
  cr_assert_eq(token_array_length(ta), 1,
               "TokenArray should have 1 element after push");
  cr_assert(!token_array_is_empty(ta),
            "TokenArray should not be empty after push");
  cr_assert_eq(token_array_at(ta, 0), TOKEN_PLUS,
               "First element should be TOKEN_PLUS");

  token_array_destroy(&ta);
  cr_assert_null(ta, "TokenArray should be NULL after destroy");
}