#include "../src/common/error_reporter.h"
#include "../src/common/file_reader.h"
#include "../src/frontend/lexer/lexer.h"
#include "../src/frontend/parser/parser.h"
#include "token.h"
#include <criterion/criterion.h>
#include <stdio.h>
#include <string.h>

// Helper function to parse a string and check for grammar errors
static AST parse_string_with_errors(const char *input) {
  // Clear any existing errors before the test
  er_free();

  FileReader fr = filereader_init_from_string(input);
  TokenArray ta = lexer_parse(fr);
  AST ast = ast_parse(ta);

  filereader_destroy(&fr);
  token_array_destroy(&ta);
  return ast;
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

// Helper function to find grammar error with specific text
static bool find_grammar_error_with_text(const char *text) {
  uint32_t count = get_error_count();
  for (uint32_t i = 0; i < count; i++) {
    CompilerError error = get_error_at(i);
    if (error.type == ERROR_GRAMMAR && error_contains_text(&error, text)) {
      return true;
    }
  }
  return false;
}

// =========================
// PRINT STATEMENT ERROR TESTS
// =========================

Test(error_grammar_test, print_missing_expression_or_string) {
  AST ast = parse_string_with_errors("PRINT");

  cr_assert_eq(get_error_count(), 1,
               "Should have 1 error for incomplete PRINT");
  // PRINT without argument should try to parse expression and fail

  ast_destroy(&ast);
  er_free();
}

// =========================
// IF STATEMENT ERROR TESTS
// =========================

Test(error_grammar_test, if_missing_comparison) {
  AST ast = parse_string_with_errors("IF");

  cr_assert_gt(get_error_count(), 0, "Should have errors for incomplete IF");
  cr_assert(find_grammar_error_with_text("comparison"),
            "Should have expected comparison error");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, if_missing_then) {
  AST ast = parse_string_with_errors("IF x > 5");

  cr_assert_gt(get_error_count(), 0, "Should have errors for IF without THEN");
  cr_assert(find_grammar_error_with_text("THEN"),
            "Should mention missing THEN");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, if_missing_endif) {
  AST ast = parse_string_with_errors("IF x > 5 THEN\nPRINT x");

  cr_assert_gt(get_error_count(), 0, "Should have errors for IF without ENDIF");
  cr_assert(find_grammar_error_with_text("ENDIF"),
            "Should mention missing ENDIF");

  ast_destroy(&ast);
  er_free();
}

// =========================
// WHILE STATEMENT ERROR TESTS
// =========================

Test(error_grammar_test, while_missing_comparison) {
  AST ast = parse_string_with_errors("WHILE");

  cr_assert_gt(get_error_count(), 0, "Should have errors for incomplete WHILE");
  cr_assert(find_grammar_error_with_text("comparison"),
            "Should mention missing comparison");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, while_missing_repeat) {
  AST ast = parse_string_with_errors("WHILE x < 10");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for WHILE without REPEAT");
  cr_assert(find_grammar_error_with_text("Expected REPEAT"),
            "Should mention missing REPEAT");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, while_missing_endwhile) {
  AST ast = parse_string_with_errors("WHILE x < 10 REPEAT\nPRINT x");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for WHILE without ENDWHILE");
  cr_assert(find_grammar_error_with_text("ENDWHILE"),
            "Should mention missing ENDWHILE");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, while_body_errors) {
  AST ast = parse_string_with_errors(
      "WHILE x < 10 REPEAT\nINVALID_STATEMENT\nENDWHILE");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for invalid statements in WHILE body");
  // Should have error for invalid statement - either "body" or "Unknown
  // statement"
  bool found_relevant_error = find_grammar_error_with_text("body") ||
                              find_grammar_error_with_text("Unknown statement");
  cr_assert(found_relevant_error,
            "Should mention body errors or unknown statement");

  ast_destroy(&ast);
  er_free();
}

// =========================
// LABEL STATEMENT ERROR TESTS
// =========================

Test(error_grammar_test, label_missing_identifier) {
  AST ast = parse_string_with_errors("LABEL");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for LABEL without identifier");
  cr_assert(find_grammar_error_with_text("Expected an identifier after LABEL"),
            "Should mention missing identifier");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, label_invalid_identifier) {
  AST ast = parse_string_with_errors("LABEL 123");

  cr_assert_gt(
      get_error_count(), 0,
      "Should have errors for LABEL with number instead of identifier");
  cr_assert(find_grammar_error_with_text("Expected an identifier after LABEL"),
            "Should mention invalid identifier");

  ast_destroy(&ast);
  er_free();
}

// =========================
// GOTO STATEMENT ERROR TESTS
// =========================

Test(error_grammar_test, goto_missing_identifier) {
  AST ast = parse_string_with_errors("GOTO");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for GOTO without identifier");
  cr_assert(find_grammar_error_with_text("Expected an identifier after GOTO"),
            "Should mention missing identifier");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, goto_invalid_identifier) {
  AST ast = parse_string_with_errors("GOTO +");

  cr_assert_gt(
      get_error_count(), 0,
      "Should have errors for GOTO with operator instead of identifier");
  cr_assert(find_grammar_error_with_text("Expected an identifier after GOTO"),
            "Should mention invalid identifier");

  ast_destroy(&ast);
  er_free();
}

// =========================
// LET STATEMENT ERROR TESTS
// =========================

Test(error_grammar_test, let_missing_variable) {
  AST ast = parse_string_with_errors("LET");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for LET without variable");
  cr_assert(find_grammar_error_with_text("Expected a variable name after LET"),
            "Should mention missing variable");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, let_missing_equals) {
  AST ast = parse_string_with_errors("LET x");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for LET without equals");
  cr_assert(find_grammar_error_with_text("Expected \"=\" after variable name"),
            "Should mention missing equals");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, let_missing_expression) {
  AST ast = parse_string_with_errors("LET x =");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for LET without expression");
  cr_assert(find_grammar_error_with_text("Expected an expression after \"=\""),
            "Should mention missing expression");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, let_invalid_variable) {
  AST ast = parse_string_with_errors("LET 123 = 456");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for LET with number as variable");
  cr_assert(find_grammar_error_with_text("Expected a variable name after LET"),
            "Should mention invalid variable");

  ast_destroy(&ast);
  er_free();
}

// =========================
// INPUT STATEMENT ERROR TESTS
// =========================

Test(error_grammar_test, input_missing_variable) {
  AST ast = parse_string_with_errors("INPUT");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for INPUT without variable");
  cr_assert(
      find_grammar_error_with_text("Expected a variable name after INPUT"),
      "Should mention missing variable");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, input_invalid_variable) {
  AST ast = parse_string_with_errors("INPUT \"string\"");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for INPUT with string instead of variable");
  cr_assert(
      find_grammar_error_with_text("Expected a variable name after INPUT"),
      "Should mention invalid variable");

  ast_destroy(&ast);
  er_free();
}

// =========================
// UNKNOWN STATEMENT ERROR TESTS
// =========================

Test(error_grammar_test, unknown_statement_single) {
  AST ast = parse_string_with_errors("INVALID");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for unknown statement");
  cr_assert(find_grammar_error_with_text("Unknown statement"),
            "Should mention unknown statement");
  cr_assert(find_grammar_error_with_text("PRINT"),
            "Should list PRINT as valid statement");
  cr_assert(find_grammar_error_with_text("IF"),
            "Should list IF as valid statement");
  cr_assert(find_grammar_error_with_text("WHILE"),
            "Should list WHILE as valid statement");
  cr_assert(find_grammar_error_with_text("LABEL"),
            "Should list LABEL as valid statement");
  cr_assert(find_grammar_error_with_text("GOTO"),
            "Should list GOTO as valid statement");
  cr_assert(find_grammar_error_with_text("LET"),
            "Should list LET as valid statement");
  cr_assert(find_grammar_error_with_text("INPUT"),
            "Should list INPUT as valid statement");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, unknown_statement_multiple) {
  AST ast = parse_string_with_errors("INVALID1\nINVALID2\nINVALID3");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for multiple unknown statements");

  // Should have at least one "Unknown statement" error
  bool found_unknown = false;
  for (uint32_t i = 0; i < get_error_count(); i++) {
    CompilerError error = get_error_at(i);
    if (error.type == ERROR_GRAMMAR &&
        error_contains_text(&error, "Unknown statement")) {
      found_unknown = true;
      break;
    }
  }
  cr_assert(found_unknown, "Should find at least one unknown statement error");

  ast_destroy(&ast);
  er_free();
}

// =========================
// EXPRESSION ERROR TESTS
// =========================

Test(error_grammar_test, invalid_expression_in_let) {
  AST ast = parse_string_with_errors("LET x = +");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for invalid expression");
  cr_assert(find_grammar_error_with_text("Expected an expression"),
            "Should mention missing expression");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, invalid_expression_in_print) {
  AST ast = parse_string_with_errors("PRINT *");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for invalid expression in PRINT");

  ast_destroy(&ast);
  er_free();
}

// =========================
// COMPARISON ERROR TESTS
// =========================

Test(error_grammar_test, invalid_comparison_in_if) {
  AST ast = parse_string_with_errors("IF x THEN");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for incomplete comparison");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, invalid_comparison_in_while) {
  AST ast = parse_string_with_errors("WHILE x REPEAT");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for incomplete comparison");
  cr_assert(find_grammar_error_with_text("comparison"),
            "Should mention missing comparison");

  ast_destroy(&ast);
  er_free();
}

// =========================
// END OF FILE ERROR TESTS
// =========================

Test(error_grammar_test, unexpected_end_of_file) {
  AST ast = parse_string_with_errors("");

  // Empty file should not cause grammar errors, only if we expect something
  cr_assert_eq(get_error_count(), 0,
               "Empty file should not cause grammar errors");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, incomplete_statement_at_eof) {
  AST ast = parse_string_with_errors("LET x");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for incomplete statement at EOF");

  ast_destroy(&ast);
  er_free();
}

// =========================
// COMPLEX ERROR SCENARIOS
// =========================

Test(error_grammar_test, mixed_valid_and_invalid_statements) {
  AST ast =
      parse_string_with_errors("PRINT \"hello\"\nINVALID\nLET x = 42\nGOTO");

  cr_assert_gt(
      get_error_count(), 0,
      "Should have errors for invalid statements mixed with valid ones");

  // Should have error for INVALID statement
  cr_assert(find_grammar_error_with_text("Unknown statement"),
            "Should have unknown statement error");

  // Should have error for incomplete GOTO
  cr_assert(find_grammar_error_with_text("Expected an identifier after GOTO"),
            "Should have incomplete GOTO error");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, nested_statement_errors) {
  AST ast = parse_string_with_errors(
      "IF x > 5 THEN\n  INVALID_NESTED\n  LET y\nENDIF");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for invalid nested statements");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, multiple_errors_same_statement) {
  AST ast = parse_string_with_errors("LET 123 + 456");

  cr_assert_gt(get_error_count(), 0,
               "Should have multiple errors for badly formed LET statement");

  ast_destroy(&ast);
  er_free();
}

// =========================
// MALFORMED EXPRESSION TESTS
// =========================

Test(error_grammar_test, malformed_expression_consecutive_operators) {
  AST ast = parse_string_with_errors("LET x = 5 + / 3");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for consecutive, non-unary operators");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, malformed_expression_trailing_operator) {
  AST ast = parse_string_with_errors("LET x = 5 +");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for trailing operator");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, malformed_expression_leading_binary_operator) {
  AST ast = parse_string_with_errors("LET x = * 5");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for leading binary operator");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, malformed_unary_without_operand) {
  AST ast = parse_string_with_errors("LET x = +");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for unary operator without operand");

  ast_destroy(&ast);
  er_free();
}

// =========================
// MALFORMED COMPARISON TESTS
// =========================

Test(error_grammar_test, comparison_missing_left_operand) {
  AST ast = parse_string_with_errors("IF > 5 THEN ENDIF");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for comparison missing left operand");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, comparison_missing_right_operand) {
  AST ast = parse_string_with_errors("IF x > THEN ENDIF");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for comparison missing right operand");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, comparison_consecutive_operators) {
  AST ast = parse_string_with_errors("IF x == == 5 THEN ENDIF");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for consecutive comparison operators");

  ast_destroy(&ast);
  er_free();
}

// =========================
// CONTROL FLOW MISMATCH TESTS
// =========================

Test(error_grammar_test, endif_without_if) {
  AST ast = parse_string_with_errors("ENDIF");

  cr_assert_gt(get_error_count(), 0, "Should have errors for ENDIF without IF");
  cr_assert(find_grammar_error_with_text("Unknown statement"),
            "Should report unknown statement");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, endwhile_without_while) {
  AST ast = parse_string_with_errors("ENDWHILE");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for ENDWHILE without WHILE");
  cr_assert(find_grammar_error_with_text("Unknown statement"),
            "Should report unknown statement");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, mismatched_nesting) {
  AST ast = parse_string_with_errors(
      "IF x > 0 THEN\nWHILE y < 10 REPEAT\nENDIF\nENDWHILE");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for mismatched nesting");

  ast_destroy(&ast);
  er_free();
}

// =========================
// TOKEN TYPE MISMATCH TESTS
// =========================

Test(error_grammar_test, operator_as_identifier) {
  AST ast = parse_string_with_errors("LABEL +");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for operator used as identifier");
  cr_assert(find_grammar_error_with_text("identifier"),
            "Should mention expected identifier");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, keyword_as_identifier) {
  AST ast = parse_string_with_errors("LET IF = 5");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for keyword used as identifier");
  cr_assert(find_grammar_error_with_text("variable"),
            "Should mention expected variable");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, string_as_identifier) {
  AST ast = parse_string_with_errors("GOTO \"label\"");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for string used as identifier");
  cr_assert(find_grammar_error_with_text("identifier"),
            "Should mention expected identifier");

  ast_destroy(&ast);
  er_free();
}

// =========================
// COMPLEX NESTED ERROR TESTS
// =========================

Test(error_grammar_test, deeply_nested_with_errors) {
  AST ast = parse_string_with_errors(
      "IF x > 0 THEN\n  IF y > 0 THEN\n    INVALID\n  ENDIF\nENDIF");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors in deeply nested structure");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, multiple_errors_different_levels) {
  AST ast = parse_string_with_errors(
      "INVALID1\nIF x > 0 THEN\n  INVALID2\n  LET y\nENDIF\nINVALID3");

  cr_assert_gt(get_error_count(), 2,
               "Should have multiple errors at different levels");

  ast_destroy(&ast);
  er_free();
}

// =========================
// EXPRESSION COMPLEXITY TESTS
// =========================

Test(error_grammar_test, print_with_invalid_expression) {
  AST ast = parse_string_with_errors("PRINT 5 + * 3");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for invalid expression in PRINT");

  ast_destroy(&ast);
  er_free();
}

// =========================
// BOUNDARY CONDITION TESTS
// =========================

Test(error_grammar_test, statement_with_only_keyword) {
  AST ast = parse_string_with_errors("LET\nPRINT\nIF");

  cr_assert_gt(get_error_count(), 2,
               "Should have multiple errors for incomplete statements");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, mixed_valid_invalid_complex) {
  AST ast = parse_string_with_errors(
      "LET x = 42\nINVALID\nPRINT x\nLET y\nGOTO somewhere");

  cr_assert_gt(
      get_error_count(), 1,
      "Should have errors for invalid statements mixed with valid ones");

  ast_destroy(&ast);
  er_free();
}

// =========================
// BLOCK PAIRING TESTS
// =========================

Test(error_grammar_test, if_missing_endif_simple) {
  AST ast = parse_string_with_errors("IF x > 0 THEN\nPRINT x");

  cr_assert_gt(get_error_count(), 0, "Should have errors for IF without ENDIF");
  cr_assert(find_grammar_error_with_text("ENDIF"),
            "Should mention missing ENDIF");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, if_missing_endif_with_statements) {
  AST ast =
      parse_string_with_errors("IF x > 0 THEN\nPRINT x\nLET y = 5\nPRINT y");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for IF with statements but no ENDIF");
  cr_assert(find_grammar_error_with_text("ENDIF"),
            "Should mention missing ENDIF");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, while_missing_endwhile_simple) {
  AST ast = parse_string_with_errors("WHILE x < 10 REPEAT\nPRINT x");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for WHILE without ENDWHILE");
  cr_assert(find_grammar_error_with_text("ENDWHILE"),
            "Should mention missing ENDWHILE");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, while_missing_endwhile_with_statements) {
  AST ast = parse_string_with_errors(
      "WHILE x < 10 REPEAT\nPRINT x\nLET x = x + 1\nPRINT \"loop\"");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for WHILE with statements but no ENDWHILE");
  cr_assert(find_grammar_error_with_text("ENDWHILE"),
            "Should mention missing ENDWHILE");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, nested_if_missing_inner_endif) {
  AST ast = parse_string_with_errors(
      "IF x > 0 THEN\n  IF y > 0 THEN\n    PRINT \"positive\"\nENDIF");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for nested IF missing inner ENDIF");
  cr_assert(find_grammar_error_with_text("ENDIF"),
            "Should mention missing ENDIF");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, nested_if_missing_outer_endif) {
  AST ast = parse_string_with_errors(
      "IF x > 0 THEN\n  IF y > 0 THEN\n    PRINT \"positive\"\n  ENDIF");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for nested IF missing outer ENDIF");
  cr_assert(find_grammar_error_with_text("ENDIF"),
            "Should mention missing ENDIF");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, nested_while_missing_inner_endwhile) {
  AST ast = parse_string_with_errors("WHILE i < 5 REPEAT\n  WHILE j < 3 "
                                     "REPEAT\n    PRINT \"nested\"\nENDWHILE");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for nested WHILE missing inner ENDWHILE");
  cr_assert(find_grammar_error_with_text("ENDWHILE"),
            "Should mention missing ENDWHILE");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, nested_while_missing_outer_endwhile) {
  AST ast =
      parse_string_with_errors("WHILE i < 5 REPEAT\n  WHILE j < 3 REPEAT\n    "
                               "PRINT \"nested\"\n  ENDWHILE");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for nested WHILE missing outer ENDWHILE");
  cr_assert(find_grammar_error_with_text("ENDWHILE"),
            "Should mention missing ENDWHILE");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, mixed_nesting_if_while_missing_endif) {
  AST ast = parse_string_with_errors(
      "IF x > 0 THEN\n  WHILE y < 10 REPEAT\n    PRINT y\n  ENDWHILE");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for IF containing WHILE but missing ENDIF");
  cr_assert(find_grammar_error_with_text("ENDIF"),
            "Should mention missing ENDIF");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, mixed_nesting_while_if_missing_endwhile) {
  AST ast = parse_string_with_errors(
      "WHILE i < 5 REPEAT\n  IF x > 0 THEN\n    PRINT x\n  ENDIF");

  cr_assert_gt(
      get_error_count(), 0,
      "Should have errors for WHILE containing IF but missing ENDWHILE");
  cr_assert(find_grammar_error_with_text("ENDWHILE"),
            "Should mention missing ENDWHILE");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, wrong_closing_token_if_endwhile) {
  AST ast = parse_string_with_errors("IF x > 0 THEN\nPRINT x\nENDWHILE");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for IF closed with ENDWHILE");
  // Should error on ENDWHILE as unknown statement since IF expects ENDIF
  cr_assert(find_grammar_error_with_text("IF"),
            "Should mention IF for wrong closing token");
  cr_assert(find_grammar_error_with_text("ENDIF"),
            "Should mention ENDIF for wrong closing token");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, wrong_closing_token_while_endif) {
  AST ast = parse_string_with_errors("WHILE x < 10 REPEAT\nPRINT x\nENDIF");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for WHILE closed with ENDIF");
  // Should error on ENDIF as unknown statement since WHILE expects ENDWHILE
  cr_assert(find_grammar_error_with_text("WHILE"),
            "Should mention WHILE for wrong closing token");
  cr_assert(find_grammar_error_with_text("ENDWHILE"),
            "Should mention ENDWHILE for wrong closing token");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, multiple_missing_closing_tokens) {
  AST ast =
      parse_string_with_errors("IF x > 0 THEN\n  WHILE y < 5 REPEAT\n    IF z "
                               "> 0 THEN\n      PRINT \"all positive\"");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for multiple missing closing tokens");
  // Should find at least one error about missing closing tokens
  bool found_closing_error = find_grammar_error_with_text("ENDIF") ||
                             find_grammar_error_with_text("ENDWHILE") ||
                             find_grammar_error_with_text("end of file");
  cr_assert(found_closing_error, "Should mention missing closing tokens");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, deeply_nested_missing_multiple_closers) {
  AST ast = parse_string_with_errors(
      "IF a > 0 THEN\n  WHILE b < 10 REPEAT\n    IF c > 0 THEN\n      WHILE d "
      "< 5 REPEAT\n        PRINT \"deep\"");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for deeply nested structure missing "
               "multiple closers");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, interleaved_blocks_wrong_order) {
  AST ast = parse_string_with_errors(
      "IF x > 0 THEN\n  WHILE y < 10 REPEAT\nENDIF\nENDWHILE");

  cr_assert_gt(
      get_error_count(), 0,
      "Should have errors for interleaved blocks closed in wrong order");

  ast_destroy(&ast);
  er_free();
}

// Test that properly paired blocks don't generate errors
Test(error_grammar_test, properly_paired_if_endif) {
  AST ast = parse_string_with_errors("IF x > 0 THEN\nPRINT x\nENDIF");

  cr_assert_eq(get_error_count(), 0,
               "Should have no errors for properly paired IF/ENDIF");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, properly_paired_while_endwhile) {
  AST ast = parse_string_with_errors(
      "WHILE x < 10 REPEAT\nPRINT x\nLET x = x + 1\nENDWHILE");

  cr_assert_eq(get_error_count(), 0,
               "Should have no errors for properly paired WHILE/ENDWHILE");

  ast_destroy(&ast);
  er_free();
}

Test(error_grammar_test, properly_nested_blocks) {
  AST ast = parse_string_with_errors(
      "IF x > 0 THEN\n  WHILE y < 10 REPEAT\n    IF z > 0 THEN\n      PRINT "
      "\"nested\"\n    ENDIF\n  ENDWHILE\nENDIF");

  cr_assert_eq(get_error_count(), 0,
               "Should have no errors for properly nested and paired blocks");

  ast_destroy(&ast);
  er_free();
}

// =========================
// ERROR RECOVERY TESTS
// =========================

Test(error_grammar_test, error_recovery_continues_parsing) {
  AST ast = parse_string_with_errors(
      "INVALID1\nPRINT \"hello\"\nINVALID2\nLET x = 42");

  cr_assert_gt(get_error_count(), 0,
               "Should have errors for invalid statements");

  // Parser should recover and continue parsing valid statements
  // This test ensures the parser doesn't completely fail on first error

  ast_destroy(&ast);
  er_free();
}
