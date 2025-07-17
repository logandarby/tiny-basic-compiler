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
    enum TOKEN actual = token_array_at(ta, i).type;
    cr_assert_eq(actual, expected[i], "Token %zu: expected %d, got %d", i,
                 expected[i], actual);
  }
}

// Helper function to check token text data
// static void assert_token_text_equal(TokenArray ta, size_t index,
//                                     const char *expected_text) {
//   cr_assert_lt(index, token_array_length(ta), "Token index %zu out of
//   bounds",
//                index);

//   Token token = token_array_at(ta, index);

//   if (expected_text == NULL) {
//     cr_assert_null(token.text, "Token %zu: expected NULL text, got '%s'",
//     index,
//                    token.text ? token.text : "NULL");
//   } else {
//     cr_assert_not_null(token.text, "Token %zu: expected text '%s', got NULL",
//                        index, expected_text);
//     cr_assert_str_eq(token.text, expected_text,
//                      "Token %zu: expected text '%s', got '%s'", index,
//                      expected_text, token.text);
//   }
// }

// Helper function to check both token types and text data
static void assert_tokens_and_text_equal(TokenArray ta,
                                         enum TOKEN *expected_types,
                                         const char **expected_texts,
                                         size_t expected_count) {
  cr_assert_eq(token_array_length(ta), expected_count,
               "Expected %zu tokens, got %zu", expected_count,
               token_array_length(ta));

  for (size_t i = 0; i < expected_count; i++) {
    Token token = token_array_at(ta, i);

    // Check token type
    cr_assert_eq(token.type, expected_types[i],
                 "Token %zu: expected type %d, got %d", i, expected_types[i],
                 token.type);

    // Check token text
    if (expected_texts[i] == NULL) {
      cr_assert_null(token.text, "Token %zu: expected NULL text, got '%s'", i,
                     token.text ? token.text : "NULL");
    } else {
      cr_assert_not_null(token.text, "Token %zu: expected text '%s', got NULL",
                         i, expected_texts[i]);
      cr_assert_str_eq(token.text, expected_texts[i],
                       "Token %zu: expected text '%s', got '%s'", i,
                       expected_texts[i], token.text);
    }
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
// OPERATOR TEXT DATA TESTS
// =========================

Test(lexer, single_char_arithmetic_operators_text) {
  TokenArray ta = parse_string("+ - * /");

  enum TOKEN expected_types[] = {TOKEN_PLUS, TOKEN_MINUS, TOKEN_MULT,
                                 TOKEN_DIV};
  const char *expected_texts[] = {NULL, NULL, NULL, NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 4);

  token_array_destroy(&ta);
}

Test(lexer, single_char_comparison_operators_text) {
  TokenArray ta = parse_string("> < =");

  enum TOKEN expected_types[] = {TOKEN_GT, TOKEN_LT, TOKEN_EQ};
  const char *expected_texts[] = {NULL, NULL, NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 3);

  token_array_destroy(&ta);
}

Test(lexer, double_char_comparison_operators_text) {
  TokenArray ta = parse_string(">= <= == !=");

  enum TOKEN expected_types[] = {TOKEN_GTE, TOKEN_LTE, TOKEN_EQEQ, TOKEN_NOTEQ};
  const char *expected_texts[] = {NULL, NULL, NULL, NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 4);

  token_array_destroy(&ta);
}

Test(lexer, double_char_logic_operators_text) {
  TokenArray ta = parse_string("&& ||");

  enum TOKEN expected_types[] = {TOKEN_AND, TOKEN_OR};
  const char *expected_texts[] = {NULL, NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 2);

  token_array_destroy(&ta);
}

Test(lexer, all_operators_text_null) {
  TokenArray ta = parse_string("+ - * / > < >= <= = == != ! && ||");

  enum TOKEN expected_types[] = {
      TOKEN_PLUS,  TOKEN_MINUS, TOKEN_MULT, TOKEN_DIV, TOKEN_GT,
      TOKEN_LT,    TOKEN_GTE,   TOKEN_LTE,  TOKEN_EQ,  TOKEN_EQEQ,
      TOKEN_NOTEQ, TOKEN_NOT,   TOKEN_AND,  TOKEN_OR};
  const char *expected_texts[] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL, NULL, NULL, NULL, NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 14);

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
    cr_assert_eq(token_array_at(ta, 0).type, TOKEN_UNKNOWN,
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

  token_array_push_simple(ta, TOKEN_PLUS);
  cr_assert_eq(token_array_length(ta), 1,
               "TokenArray should have 1 element after push");
  cr_assert(!token_array_is_empty(ta),
            "TokenArray should not be empty after push");
  cr_assert_eq(token_array_at(ta, 0).type, TOKEN_PLUS,
               "First element should be TOKEN_PLUS");

  token_array_destroy(&ta);
  cr_assert_null(ta, "TokenArray should be NULL after destroy");
}

// =========================
// NUMBER TOKENIZATION TESTS
// =========================

Test(lexer, single_digit_numbers) {
  TokenArray ta = parse_string("0 1 2 3 4 5 6 7 8 9");

  enum TOKEN expected[10];
  for (int i = 0; i < 10; i++) {
    expected[i] = TOKEN_NUMBER;
  }
  assert_tokens_equal(ta, expected, 10);

  token_array_destroy(&ta);
}

Test(lexer, multi_digit_numbers) {
  TokenArray ta = parse_string("12 123 1234 12345");

  enum TOKEN expected[] = {TOKEN_NUMBER, TOKEN_NUMBER, TOKEN_NUMBER,
                           TOKEN_NUMBER};
  assert_tokens_equal(ta, expected, 4);

  token_array_destroy(&ta);
}

Test(lexer, numbers_with_leading_zeros) {
  TokenArray ta = parse_string("01 001 0123 00000");

  enum TOKEN expected[] = {TOKEN_NUMBER, TOKEN_NUMBER, TOKEN_NUMBER,
                           TOKEN_NUMBER};
  assert_tokens_equal(ta, expected, 4);

  token_array_destroy(&ta);
}

Test(lexer, large_numbers) {
  TokenArray ta = parse_string("999999999 1234567890 987654321");

  enum TOKEN expected[] = {TOKEN_NUMBER, TOKEN_NUMBER, TOKEN_NUMBER};
  assert_tokens_equal(ta, expected, 3);

  token_array_destroy(&ta);
}

Test(lexer, numbers_with_whitespace) {
  TokenArray ta = parse_string("  42   123\t\t456\n\n789  ");

  enum TOKEN expected[] = {TOKEN_NUMBER, TOKEN_NUMBER, TOKEN_NUMBER,
                           TOKEN_NUMBER};
  assert_tokens_equal(ta, expected, 4);

  token_array_destroy(&ta);
}

Test(lexer, zero_variations) {
  TokenArray ta = parse_string("0 00 000 0000");

  enum TOKEN expected[] = {TOKEN_NUMBER, TOKEN_NUMBER, TOKEN_NUMBER,
                           TOKEN_NUMBER};
  assert_tokens_equal(ta, expected, 4);

  token_array_destroy(&ta);
}

// =========================
// NUMBER TEXT DATA TESTS
// =========================

Test(lexer, single_digit_number_text) {
  TokenArray ta = parse_string("0 1 5 9");

  enum TOKEN expected_types[] = {TOKEN_NUMBER, TOKEN_NUMBER, TOKEN_NUMBER,
                                 TOKEN_NUMBER};
  const char *expected_texts[] = {"0", "1", "5", "9"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 4);

  token_array_destroy(&ta);
}

Test(lexer, multi_digit_number_text) {
  TokenArray ta = parse_string("12 123 1234 12345");

  enum TOKEN expected_types[] = {TOKEN_NUMBER, TOKEN_NUMBER, TOKEN_NUMBER,
                                 TOKEN_NUMBER};
  const char *expected_texts[] = {"12", "123", "1234", "12345"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 4);

  token_array_destroy(&ta);
}

Test(lexer, numbers_with_leading_zeros_text) {
  TokenArray ta = parse_string("01 001 0123 00000");

  enum TOKEN expected_types[] = {TOKEN_NUMBER, TOKEN_NUMBER, TOKEN_NUMBER,
                                 TOKEN_NUMBER};
  const char *expected_texts[] = {"01", "001", "0123", "00000"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 4);

  token_array_destroy(&ta);
}

Test(lexer, large_numbers_text) {
  TokenArray ta = parse_string("999999999 1234567890 987654321");

  enum TOKEN expected_types[] = {TOKEN_NUMBER, TOKEN_NUMBER, TOKEN_NUMBER};
  const char *expected_texts[] = {"999999999", "1234567890", "987654321"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 3);

  token_array_destroy(&ta);
}

Test(lexer, zero_variations_text) {
  TokenArray ta = parse_string("0 00 000 0000");

  enum TOKEN expected_types[] = {TOKEN_NUMBER, TOKEN_NUMBER, TOKEN_NUMBER,
                                 TOKEN_NUMBER};
  const char *expected_texts[] = {"0", "00", "000", "0000"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 4);

  token_array_destroy(&ta);
}

// =========================
// IDENTIFIER TOKENIZATION TESTS
// =========================

Test(lexer, single_letter_identifiers) {
  TokenArray ta = parse_string("a b c x y z A B C X Y Z");

  enum TOKEN expected[12];
  for (int i = 0; i < 12; i++) {
    expected[i] = TOKEN_IDENT;
  }
  assert_tokens_equal(ta, expected, 12);

  token_array_destroy(&ta);
}

Test(lexer, multi_letter_identifiers) {
  TokenArray ta = parse_string("abc xyz hello world variable temp");

  enum TOKEN expected[] = {TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT,
                           TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 6);

  token_array_destroy(&ta);
}

Test(lexer, mixed_case_identifiers) {
  TokenArray ta = parse_string("Abc XyZ HeLLo WoRlD VaRiAbLe");

  enum TOKEN expected[] = {TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT,
                           TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 5);

  token_array_destroy(&ta);
}

Test(lexer, long_identifiers) {
  TokenArray ta = parse_string(
      "verylongidentifiername ANOTHERLONGIDENTIFIER mixedCaseVeryLongName");

  enum TOKEN expected[] = {TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 3);

  token_array_destroy(&ta);
}

Test(lexer, identifiers_with_whitespace) {
  TokenArray ta = parse_string("  var1   var2\t\tvar3\n\nvar4  ");

  enum TOKEN expected[] = {TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 4);

  token_array_destroy(&ta);
}

// =========================
// IDENTIFIER TEXT DATA TESTS
// =========================

Test(lexer, single_letter_identifiers_text) {
  TokenArray ta = parse_string("a b c x y z A B C X Y Z");

  enum TOKEN expected_types[12];
  const char *expected_texts[] = {"a", "b", "c", "x", "y", "z",
                                  "A", "B", "C", "X", "Y", "Z"};
  for (int i = 0; i < 12; i++) {
    expected_types[i] = TOKEN_IDENT;
  }
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 12);

  token_array_destroy(&ta);
}

Test(lexer, multi_letter_identifiers_text) {
  TokenArray ta = parse_string("abc xyz hello world variable temp");

  enum TOKEN expected_types[] = {TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT,
                                 TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT};
  const char *expected_texts[] = {"abc",   "xyz",      "hello",
                                  "world", "variable", "temp"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 6);

  token_array_destroy(&ta);
}

Test(lexer, mixed_case_identifiers_text) {
  TokenArray ta = parse_string("Abc XyZ HeLLo WoRlD VaRiAbLe");

  enum TOKEN expected_types[] = {TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT,
                                 TOKEN_IDENT, TOKEN_IDENT};
  const char *expected_texts[] = {"Abc", "XyZ", "HeLLo", "WoRlD", "VaRiAbLe"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 5);

  token_array_destroy(&ta);
}

Test(lexer, long_identifiers_text) {
  TokenArray ta = parse_string(
      "verylongidentifiername ANOTHERLONGIDENTIFIER mixedCaseVeryLongName");

  enum TOKEN expected_types[] = {TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT};
  const char *expected_texts[] = {"verylongidentifiername",
                                  "ANOTHERLONGIDENTIFIER",
                                  "mixedCaseVeryLongName"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 3);

  token_array_destroy(&ta);
}

Test(lexer, identifiers_with_whitespace_text) {
  TokenArray ta = parse_string("  var1   var2\t\tvar3\n\nvar4  ");

  enum TOKEN expected_types[] = {TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT,
                                 TOKEN_IDENT};
  const char *expected_texts[] = {"var1", "var2", "var3", "var4"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 4);

  token_array_destroy(&ta);
}

// =========================
// KEYWORD TOKENIZATION TESTS
// =========================

Test(lexer, all_keywords_uppercase) {
  TokenArray ta = parse_string(
      "LABEL GOTO PRINT INPUT LET IF THEN ELSE ENDIF WHILE REPEAT ENDWHILE");

  enum TOKEN expected[] = {TOKEN_LABEL, TOKEN_GOTO,   TOKEN_PRINT,
                           TOKEN_INPUT, TOKEN_LET,    TOKEN_IF,
                           TOKEN_THEN,  TOKEN_ELSE,   TOKEN_ENDIF,
                           TOKEN_WHILE, TOKEN_REPEAT, TOKEN_ENDWHILE};
  assert_tokens_equal(ta, expected, 12);

  token_array_destroy(&ta);
}

Test(lexer, keywords_case_sensitivity) {
  // Keywords should be case-sensitive (uppercase only)
  TokenArray ta = parse_string("if IF then THEN else ELSE");

  enum TOKEN expected[] = {TOKEN_IDENT, TOKEN_IF,    TOKEN_IDENT,
                           TOKEN_THEN,  TOKEN_IDENT, TOKEN_ELSE};
  assert_tokens_equal(ta, expected, 6);

  token_array_destroy(&ta);
}

Test(lexer, single_keywords) {
  TokenArray ta1 = parse_string("IF");
  enum TOKEN expected1[] = {TOKEN_IF};
  assert_tokens_equal(ta1, expected1, 1);
  token_array_destroy(&ta1);

  TokenArray ta2 = parse_string("WHILE");
  enum TOKEN expected2[] = {TOKEN_WHILE};
  assert_tokens_equal(ta2, expected2, 1);
  token_array_destroy(&ta2);

  TokenArray ta3 = parse_string("PRINT");
  enum TOKEN expected3[] = {TOKEN_PRINT};
  assert_tokens_equal(ta3, expected3, 1);
  token_array_destroy(&ta3);
}

Test(lexer, control_flow_keywords) {
  TokenArray ta = parse_string("IF THEN ELSE ENDIF WHILE ENDWHILE");

  enum TOKEN expected[] = {TOKEN_IF,    TOKEN_THEN,  TOKEN_ELSE,
                           TOKEN_ENDIF, TOKEN_WHILE, TOKEN_ENDWHILE};
  assert_tokens_equal(ta, expected, 6);

  token_array_destroy(&ta);
}

Test(lexer, io_keywords) {
  TokenArray ta = parse_string("PRINT INPUT");

  enum TOKEN expected[] = {TOKEN_PRINT, TOKEN_INPUT};
  assert_tokens_equal(ta, expected, 2);

  token_array_destroy(&ta);
}

Test(lexer, assignment_keywords) {
  TokenArray ta = parse_string("LET");

  enum TOKEN expected[] = {TOKEN_LET};
  assert_tokens_equal(ta, expected, 1);

  token_array_destroy(&ta);
}

Test(lexer, jump_keywords) {
  TokenArray ta = parse_string("LABEL GOTO");

  enum TOKEN expected[] = {TOKEN_LABEL, TOKEN_GOTO};
  assert_tokens_equal(ta, expected, 2);

  token_array_destroy(&ta);
}

Test(lexer, loop_keywords) {
  TokenArray ta = parse_string("WHILE REPEAT ENDWHILE");

  enum TOKEN expected[] = {TOKEN_WHILE, TOKEN_REPEAT, TOKEN_ENDWHILE};
  assert_tokens_equal(ta, expected, 3);

  token_array_destroy(&ta);
}

// =========================
// KEYWORD TEXT DATA TESTS
// =========================

Test(lexer, all_keywords_text_null) {
  TokenArray ta = parse_string(
      "LABEL GOTO PRINT INPUT LET IF THEN ELSE ENDIF WHILE REPEAT ENDWHILE");

  enum TOKEN expected_types[] = {TOKEN_LABEL, TOKEN_GOTO,   TOKEN_PRINT,
                                 TOKEN_INPUT, TOKEN_LET,    TOKEN_IF,
                                 TOKEN_THEN,  TOKEN_ELSE,   TOKEN_ENDIF,
                                 TOKEN_WHILE, TOKEN_REPEAT, TOKEN_ENDWHILE};
  const char *expected_texts[] = {NULL, NULL, NULL, NULL, NULL, NULL,
                                  NULL, NULL, NULL, NULL, NULL, NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 12);

  token_array_destroy(&ta);
}

Test(lexer, control_flow_keywords_text) {
  TokenArray ta = parse_string("IF THEN ELSE ENDIF WHILE ENDWHILE");

  enum TOKEN expected_types[] = {TOKEN_IF,    TOKEN_THEN,  TOKEN_ELSE,
                                 TOKEN_ENDIF, TOKEN_WHILE, TOKEN_ENDWHILE};
  const char *expected_texts[] = {NULL, NULL, NULL, NULL, NULL, NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 6);

  token_array_destroy(&ta);
}

Test(lexer, io_keywords_text) {
  TokenArray ta = parse_string("PRINT INPUT");

  enum TOKEN expected_types[] = {TOKEN_PRINT, TOKEN_INPUT};
  const char *expected_texts[] = {NULL, NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 2);

  token_array_destroy(&ta);
}

Test(lexer, assignment_keywords_text) {
  TokenArray ta = parse_string("LET");

  enum TOKEN expected_types[] = {TOKEN_LET};
  const char *expected_texts[] = {NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 1);

  token_array_destroy(&ta);
}

Test(lexer, jump_keywords_text) {
  TokenArray ta = parse_string("LABEL GOTO");

  enum TOKEN expected_types[] = {TOKEN_LABEL, TOKEN_GOTO};
  const char *expected_texts[] = {NULL, NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 2);

  token_array_destroy(&ta);
}

// =========================
// MIXED TOKEN TYPE TESTS
// =========================

Test(lexer, keywords_with_operators) {
  TokenArray ta = parse_string("IF x > 10 THEN PRINT x ENDIF");

  enum TOKEN expected_types[] = {TOKEN_IF,     TOKEN_IDENT, TOKEN_GT,
                                 TOKEN_NUMBER, TOKEN_THEN,  TOKEN_PRINT,
                                 TOKEN_IDENT,  TOKEN_ENDIF};
  const char *expected_texts[] = {NULL, "x", NULL, "10", NULL, NULL, "x", NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 8);

  token_array_destroy(&ta);
}

Test(lexer, assignment_statement) {
  TokenArray ta = parse_string("LET x = 42 + y");

  enum TOKEN expected_types[] = {TOKEN_LET,    TOKEN_IDENT, TOKEN_EQ,
                                 TOKEN_NUMBER, TOKEN_PLUS,  TOKEN_IDENT};
  const char *expected_texts[] = {NULL, "x", NULL, "42", NULL, "y"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 6);

  token_array_destroy(&ta);
}

Test(lexer, loop_with_condition) {
  TokenArray ta = parse_string("WHILE i <= 100");

  enum TOKEN expected_types[] = {TOKEN_WHILE, TOKEN_IDENT, TOKEN_LTE,
                                 TOKEN_NUMBER};
  const char *expected_texts[] = {NULL, "i", NULL, "100"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 4);

  token_array_destroy(&ta);
}

Test(lexer, complex_expression) {
  TokenArray ta = parse_string("result = a + b * c - d / e");

  enum TOKEN expected_types[] = {TOKEN_IDENT, TOKEN_EQ,    TOKEN_IDENT,
                                 TOKEN_PLUS,  TOKEN_IDENT, TOKEN_MULT,
                                 TOKEN_IDENT, TOKEN_MINUS, TOKEN_IDENT,
                                 TOKEN_DIV,   TOKEN_IDENT};
  const char *expected_texts[] = {"result", NULL, "a", NULL, "b", NULL,
                                  "c",      NULL, "d", NULL, "e"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 11);

  token_array_destroy(&ta);
}

Test(lexer, conditional_with_logical_operators) {
  TokenArray ta = parse_string("IF x >= min && x <= max");

  enum TOKEN expected[] = {TOKEN_IF,  TOKEN_IDENT, TOKEN_GTE, TOKEN_IDENT,
                           TOKEN_AND, TOKEN_IDENT, TOKEN_LTE, TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 8);

  token_array_destroy(&ta);
}

Test(lexer, numbers_and_identifiers_mixed) {
  TokenArray ta = parse_string("var1 123 var2 456 var3");

  enum TOKEN expected[] = {TOKEN_IDENT, TOKEN_NUMBER, TOKEN_IDENT, TOKEN_NUMBER,
                           TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 5);

  token_array_destroy(&ta);
}

Test(lexer, keywords_and_identifiers_mixed) {
  TokenArray ta = parse_string("LET counter = 0 WHILE counter < limit");

  enum TOKEN expected[] = {TOKEN_LET,   TOKEN_IDENT, TOKEN_EQ, TOKEN_NUMBER,
                           TOKEN_WHILE, TOKEN_IDENT, TOKEN_LT, TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 8);

  token_array_destroy(&ta);
}

// =========================
// EDGE CASES AND ERROR HANDLING
// =========================

Test(lexer, adjacent_tokens_no_whitespace) {
  // Note: This tests how the lexer handles tokens that are not separated by
  // whitespace Based on the implementation, this should still work correctly as
  // each token type is recognized by character classes
  TokenArray ta = parse_string("IF x>10THEN");

  enum TOKEN expected[] = {TOKEN_IF, TOKEN_IDENT, TOKEN_GT, TOKEN_NUMBER,
                           TOKEN_THEN};
  assert_tokens_equal(ta, expected, 5);

  token_array_destroy(&ta);
}

Test(lexer, identifiers_starting_with_keyword_prefix) {
  TokenArray ta = parse_string("IFF WHILELOOP PRINTABLE");

  enum TOKEN expected[] = {TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 3);

  token_array_destroy(&ta);
}

Test(lexer, numbers_and_identifiers_adjacent) {
  TokenArray ta = parse_string("123abc 456def");

  // Based on the lexer implementation, numbers and letters are recognized
  // separately
  enum TOKEN expected[] = {TOKEN_NUMBER, TOKEN_IDENT, TOKEN_NUMBER,
                           TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 4);

  token_array_destroy(&ta);
}

Test(lexer, very_long_number) {
  TokenArray ta =
      parse_string("12345678901234567890123456789012345678901234567890");

  enum TOKEN expected[] = {TOKEN_NUMBER};
  assert_tokens_equal(ta, expected, 1);

  token_array_destroy(&ta);
}

Test(lexer, very_long_identifier) {
  TokenArray ta = parse_string(
      "verylongidentifiernamethatgoesonfarlongerthanmostpeoplewouldexpect");

  enum TOKEN expected[] = {TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 1);

  token_array_destroy(&ta);
}

Test(lexer, mixed_case_non_keywords) {
  TokenArray ta = parse_string("If Then Else While Print");

  enum TOKEN expected[] = {TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT, TOKEN_IDENT,
                           TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 5);

  token_array_destroy(&ta);
}

// =========================
// REALISTIC PROGRAM FRAGMENTS
// =========================

Test(lexer, simple_program_fragment) {
  TokenArray ta = parse_string("LET x = 10\nPRINT x");

  enum TOKEN expected[] = {TOKEN_LET,    TOKEN_IDENT, TOKEN_EQ,
                           TOKEN_NUMBER, TOKEN_PRINT, TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 6);

  token_array_destroy(&ta);
}

Test(lexer, if_statement_fragment) {
  TokenArray ta = parse_string("IF x > 0 THEN PRINT positive ENDIF");

  enum TOKEN expected[] = {TOKEN_IF,   TOKEN_IDENT, TOKEN_GT,    TOKEN_NUMBER,
                           TOKEN_THEN, TOKEN_PRINT, TOKEN_IDENT, TOKEN_ENDIF};
  assert_tokens_equal(ta, expected, 8);

  token_array_destroy(&ta);
}

Test(lexer, while_loop_fragment) {
  TokenArray ta =
      parse_string("WHILE i <= 10 REPEAT PRINT i LET i = i + 1 ENDWHILE");

  enum TOKEN expected[] = {
      TOKEN_WHILE, TOKEN_IDENT, TOKEN_LTE,    TOKEN_NUMBER,  TOKEN_REPEAT,
      TOKEN_PRINT, TOKEN_IDENT, TOKEN_LET,    TOKEN_IDENT,   TOKEN_EQ,
      TOKEN_IDENT, TOKEN_PLUS,  TOKEN_NUMBER, TOKEN_ENDWHILE};
  assert_tokens_equal(ta, expected, 14);

  token_array_destroy(&ta);
}

Test(lexer, goto_label_fragment) {
  TokenArray ta = parse_string("LABEL start PRINT hello GOTO start");

  enum TOKEN expected[] = {TOKEN_LABEL, TOKEN_IDENT, TOKEN_PRINT,
                           TOKEN_IDENT, TOKEN_GOTO,  TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 6);

  token_array_destroy(&ta);
}

Test(lexer, input_output_fragment) {
  TokenArray ta = parse_string("INPUT x PRINT x + 1");

  enum TOKEN expected[] = {TOKEN_INPUT, TOKEN_IDENT, TOKEN_PRINT,
                           TOKEN_IDENT, TOKEN_PLUS,  TOKEN_NUMBER};
  assert_tokens_equal(ta, expected, 6);

  token_array_destroy(&ta);
}

// =========================
// COMPREHENSIVE STRESS TESTS
// =========================

Test(lexer, all_token_types_mixed) {
  TokenArray ta =
      parse_string("LET result = 0 IF x >= 10 && y != 0 THEN result = x + y "
                   "ELSE result = x - y ENDIF PRINT result");

  enum TOKEN expected[] = {TOKEN_LET,   TOKEN_IDENT, TOKEN_EQ,    TOKEN_NUMBER,
                           TOKEN_IF,    TOKEN_IDENT, TOKEN_GTE,   TOKEN_NUMBER,
                           TOKEN_AND,   TOKEN_IDENT, TOKEN_NOTEQ, TOKEN_NUMBER,
                           TOKEN_THEN,  TOKEN_IDENT, TOKEN_EQ,    TOKEN_IDENT,
                           TOKEN_PLUS,  TOKEN_IDENT, TOKEN_ELSE,  TOKEN_IDENT,
                           TOKEN_EQ,    TOKEN_IDENT, TOKEN_MINUS, TOKEN_IDENT,
                           TOKEN_ENDIF, TOKEN_PRINT, TOKEN_IDENT};
  assert_tokens_equal(ta, expected, 27);

  token_array_destroy(&ta);
}

Test(lexer, repeated_identical_tokens) {
  TokenArray ta = parse_string("x x x 123 123 123 IF IF IF");

  enum TOKEN expected[] = {TOKEN_IDENT,  TOKEN_IDENT,  TOKEN_IDENT,
                           TOKEN_NUMBER, TOKEN_NUMBER, TOKEN_NUMBER,
                           TOKEN_IF,     TOKEN_IF,     TOKEN_IF};
  assert_tokens_equal(ta, expected, 9);

  token_array_destroy(&ta);
}

Test(lexer, alternating_token_types) {
  TokenArray ta = parse_string("x 1 y 2 z 3 IF 4 THEN 5");

  enum TOKEN expected[] = {TOKEN_IDENT, TOKEN_NUMBER, TOKEN_IDENT, TOKEN_NUMBER,
                           TOKEN_IDENT, TOKEN_NUMBER, TOKEN_IF,    TOKEN_NUMBER,
                           TOKEN_THEN,  TOKEN_NUMBER};
  assert_tokens_equal(ta, expected, 10);

  token_array_destroy(&ta);
}

// =========================
// MIXED TOKEN TEXT DATA TESTS
// =========================

Test(lexer, assignment_statement_text) {
  TokenArray ta = parse_string("LET x = 42 + y");

  enum TOKEN expected_types[] = {TOKEN_LET,    TOKEN_IDENT, TOKEN_EQ,
                                 TOKEN_NUMBER, TOKEN_PLUS,  TOKEN_IDENT};
  const char *expected_texts[] = {NULL, "x", NULL, "42", NULL, "y"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 6);

  token_array_destroy(&ta);
}

Test(lexer, complex_expression_text) {
  TokenArray ta = parse_string("result = a + b * c - d / e");

  enum TOKEN expected_types[] = {TOKEN_IDENT, TOKEN_EQ,    TOKEN_IDENT,
                                 TOKEN_PLUS,  TOKEN_IDENT, TOKEN_MULT,
                                 TOKEN_IDENT, TOKEN_MINUS, TOKEN_IDENT,
                                 TOKEN_DIV,   TOKEN_IDENT};
  const char *expected_texts[] = {"result", NULL, "a", NULL, "b", NULL,
                                  "c",      NULL, "d", NULL, "e"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 11);

  token_array_destroy(&ta);
}

Test(lexer, conditional_with_logical_operators_text) {
  TokenArray ta = parse_string("IF x >= min && x <= max");

  enum TOKEN expected_types[] = {TOKEN_IF,    TOKEN_IDENT, TOKEN_GTE,
                                 TOKEN_IDENT, TOKEN_AND,   TOKEN_IDENT,
                                 TOKEN_LTE,   TOKEN_IDENT};
  const char *expected_texts[] = {NULL, "x", NULL, "min",
                                  NULL, "x", NULL, "max"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 8);

  token_array_destroy(&ta);
}

Test(lexer, numbers_and_identifiers_mixed_text) {
  TokenArray ta = parse_string("var1 123 var2 456 var3");

  enum TOKEN expected_types[] = {TOKEN_IDENT, TOKEN_NUMBER, TOKEN_IDENT,
                                 TOKEN_NUMBER, TOKEN_IDENT};
  const char *expected_texts[] = {"var1", "123", "var2", "456", "var3"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 5);

  token_array_destroy(&ta);
}

Test(lexer, realistic_program_fragment_text) {
  TokenArray ta = parse_string("LET counter = 0 WHILE counter < limit");

  enum TOKEN expected_types[] = {TOKEN_LET,    TOKEN_IDENT, TOKEN_EQ,
                                 TOKEN_NUMBER, TOKEN_WHILE, TOKEN_IDENT,
                                 TOKEN_LT,     TOKEN_IDENT};
  const char *expected_texts[] = {NULL, "counter", NULL, "0",
                                  NULL, "counter", NULL, "limit"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 8);

  token_array_destroy(&ta);
}

Test(lexer, if_statement_fragment_text) {
  TokenArray ta = parse_string("IF x > 0 THEN PRINT positive ENDIF");

  enum TOKEN expected_types[] = {TOKEN_IF,     TOKEN_IDENT, TOKEN_GT,
                                 TOKEN_NUMBER, TOKEN_THEN,  TOKEN_PRINT,
                                 TOKEN_IDENT,  TOKEN_ENDIF};
  const char *expected_texts[] = {NULL, "x",  NULL,       "0",
                                  NULL, NULL, "positive", NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 8);

  token_array_destroy(&ta);
}

Test(lexer, input_output_fragment_text) {
  TokenArray ta = parse_string("INPUT x PRINT x + 1");

  enum TOKEN expected_types[] = {TOKEN_INPUT, TOKEN_IDENT, TOKEN_PRINT,
                                 TOKEN_IDENT, TOKEN_PLUS,  TOKEN_NUMBER};
  const char *expected_texts[] = {NULL, "x", NULL, "x", NULL, "1"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 6);

  token_array_destroy(&ta);
}

Test(lexer, very_long_identifier_text) {
  TokenArray ta = parse_string(
      "verylongidentifiernamethatgoesonfarlongerthanmostpeoplewouldexpect");

  enum TOKEN expected_types[] = {TOKEN_IDENT};
  const char *expected_texts[] = {
      "verylongidentifiernamethatgoesonfarlongerthanmostpeoplewouldexpect"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 1);

  token_array_destroy(&ta);
}

Test(lexer, very_long_number_text) {
  TokenArray ta =
      parse_string("12345678901234567890123456789012345678901234567890");

  enum TOKEN expected_types[] = {TOKEN_NUMBER};
  const char *expected_texts[] = {
      "12345678901234567890123456789012345678901234567890"};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 1);

  token_array_destroy(&ta);
}

Test(lexer, adjacent_tokens_no_whitespace_text) {
  TokenArray ta = parse_string("IF x>10THEN");

  enum TOKEN expected_types[] = {TOKEN_IF, TOKEN_IDENT, TOKEN_GT, TOKEN_NUMBER,
                                 TOKEN_THEN};
  const char *expected_texts[] = {NULL, "x", NULL, "10", NULL};
  assert_tokens_and_text_equal(ta, expected_types, expected_texts, 5);

  token_array_destroy(&ta);
}

// =========================
// COMPLEX NESTED STRUCTURE TESTS
// =========================

Test(lexer, complex_nested_structure) {
  TokenArray ta = parse_string("IF x > 0 THEN WHILE y < 10 REPEAT LET z = x * "
                               "y PRINT z LET y = y + 1 ENDWHILE ENDIF");

  enum TOKEN expected[] = {
      TOKEN_IF,    TOKEN_IDENT, TOKEN_GT,    TOKEN_NUMBER, TOKEN_THEN,
      TOKEN_WHILE, TOKEN_IDENT, TOKEN_LT,    TOKEN_NUMBER, TOKEN_REPEAT,
      TOKEN_LET,   TOKEN_IDENT, TOKEN_EQ,    TOKEN_IDENT,  TOKEN_MULT,
      TOKEN_IDENT, TOKEN_PRINT, TOKEN_IDENT, TOKEN_LET,    TOKEN_IDENT,
      TOKEN_EQ,    TOKEN_IDENT, TOKEN_PLUS,  TOKEN_NUMBER, TOKEN_ENDWHILE,
      TOKEN_ENDIF};
  assert_tokens_equal(ta, expected, 26);

  token_array_destroy(&ta);
}

// =========================
// BOUNDARY AND EDGE CASE TESTS
// =========================

Test(lexer, single_character_tokens) {
  TokenArray ta = parse_string("a 1 + b 2 - c 3 * d 4 / e 5");

  enum TOKEN expected[] = {
      TOKEN_IDENT,  TOKEN_NUMBER, TOKEN_PLUS,   TOKEN_IDENT, TOKEN_NUMBER,
      TOKEN_MINUS,  TOKEN_IDENT,  TOKEN_NUMBER, TOKEN_MULT,  TOKEN_IDENT,
      TOKEN_NUMBER, TOKEN_DIV,    TOKEN_IDENT,  TOKEN_NUMBER};
  assert_tokens_equal(ta, expected, 14);

  token_array_destroy(&ta);
}

Test(lexer, maximum_length_sequences) {
  // Test with very long sequences of each token type
  TokenArray ta1 =
      parse_string("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
  enum TOKEN expected1[] = {TOKEN_IDENT};
  assert_tokens_equal(ta1, expected1, 1);
  token_array_destroy(&ta1);

  TokenArray ta2 = parse_string("1234567890123456789012345678901234567890");
  enum TOKEN expected2[] = {TOKEN_NUMBER};
  assert_tokens_equal(ta2, expected2, 1);
  token_array_destroy(&ta2);
}