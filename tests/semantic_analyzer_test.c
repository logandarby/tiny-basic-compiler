#include "../src/common/error_reporter.h"
#include "../src/common/file_reader.h"
#include "../src/common/name_table.h"
#include "../src/frontend/lexer/lexer.h"
#include "../src/frontend/parser/parser.h"
#include "../src/frontend/semantic_analyzer/semantic_analyzer.h"
#include <criterion/criterion.h>

// =========================
// HELPER FUNCTIONS
// =========================

// Helper function to create a TokenArray from a string
static TokenArray parse_string(const char *input) {
  FileReader fr = filereader_init_from_string(input);
  TokenArray ta = lexer_parse(fr);
  filereader_destroy(&fr);
  return ta;
}

// Helper function to parse string, create AST, and build name table
static void setup_test_data(const char *input, AST *ast, TokenArray *ta,
                            NameTable **table) {
  *ta = parse_string(input);
  *ast = ast_parse(*ta);
  *table = name_table_collect_from_ast(ast);
}

// Helper function to cleanup test data
static void cleanup_test_data(AST *ast, TokenArray *ta, NameTable *table) {
  name_table_destroy(table);
  ast_destroy(ast);
  token_array_destroy(ta);
}

// =========================
// SUCCESS TESTS
// =========================

Test(SemanticAnalyzer, valid_program_no_errors) {
  const char *program = "LET x = 5\n"
                        "PRINT x\n"
                        "LABEL start\n"
                        "GOTO start\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Valid program should pass semantic analysis");
  cr_assert(!er_has_errors(), "Valid program should have no errors");

  cleanup_test_data(&ast, &ta, table);
}

Test(SemanticAnalyzer, empty_program) {
  const char *program = "";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Empty program should pass semantic analysis");
  cr_assert(!er_has_errors(), "Empty program should have no errors");

  cleanup_test_data(&ast, &ta, table);
}

Test(SemanticAnalyzer, only_print_strings) {
  const char *program = "PRINT \"hello\"\n"
                        "PRINT \"world\"\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Program with only string prints should pass");
  cr_assert(!er_has_errors(),
            "Program with only string prints should have no errors");

  cleanup_test_data(&ast, &ta, table);
}

Test(SemanticAnalyzer, variable_defined_before_use) {
  const char *program = "LET x = 10\n"
                        "LET y = x + 5\n"
                        "PRINT x\n"
                        "PRINT y\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Variables defined before use should pass");
  cr_assert(!er_has_errors(),
            "Variables defined before use should have no errors");

  cleanup_test_data(&ast, &ta, table);
}

Test(SemanticAnalyzer, labels_and_gotos_valid) {
  const char *program = "LABEL loop\n"
                        "LET x = 5\n"
                        "PRINT x\n"
                        "GOTO loop\n"
                        "LABEL end\n"
                        "GOTO end\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Valid labels and gotos should pass");
  cr_assert(!er_has_errors(), "Valid labels and gotos should have no errors");

  cleanup_test_data(&ast, &ta, table);
}

Test(SemanticAnalyzer, control_flow_with_variables) {
  const char *program = "LET x = 10\n"
                        "IF x > 5 THEN\n"
                        "  LET y = x * 2\n"
                        "  PRINT y\n"
                        "ENDIF\n"
                        "WHILE x > 0 REPEAT\n"
                        "  PRINT x\n"
                        "  LET x = x - 1\n"
                        "ENDWHILE\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Control flow with proper variable usage should pass");
  cr_assert(!er_has_errors(),
            "Control flow with proper variable usage should have no errors");

  cleanup_test_data(&ast, &ta, table);
}

// =========================
// UNDEFINED VARIABLE TESTS
// =========================

Test(SemanticAnalyzer, undefined_variable_in_expression) {
  const char *program = "PRINT x\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(), "Undefined variable should generate error");
  cr_assert_eq(er_get_error_count(), 1, "Should have exactly one error");

  CompilerError error = er_get_error_at(0);
  cr_assert_eq(error.type, ERROR_SEMANTIC, "Error should be semantic type");

  cleanup_test_data(&ast, &ta, table);
  er_free(); // Clean up error state for next test
}

Test(SemanticAnalyzer, undefined_variable_in_assignment) {
  const char *program = "LET y = x + 5\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Undefined variable in assignment should generate error");
  cr_assert_eq(er_get_error_count(), 1, "Should have exactly one error");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, undefined_variable_in_comparison) {
  const char *program = "IF x > 5 THEN\n"
                        "ENDIF\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Undefined variable in comparison should generate error");
  cr_assert_eq(er_get_error_count(), 1, "Should have exactly one error");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, multiple_undefined_variables) {
  const char *program = "LET result = x + y * z\n"
                        "PRINT result\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Multiple undefined variables should generate errors");
  cr_assert_eq(er_get_error_count(), 3, "Should have three errors for x, y, z");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, undefined_variable_in_input_context) {
  const char *program = "INPUT x\n"
                        "PRINT x\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "INPUT creates variable but doesn't define it before use check");
  cr_assert_eq(er_get_error_count(), 2,
               "Should have two errors for x in PRINT");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

// =========================
// VARIABLE USE BEFORE DEFINITION TESTS
// =========================

Test(SemanticAnalyzer, variable_used_before_definition_same_line) {
  const char *program = "LET x = x + 1\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Variable used before definition should generate error");
  cr_assert_eq(er_get_error_count(), 1, "Should have exactly one error");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, variable_defined_and_referenced_same_line) {
  const char *program = "LET x = 1 LET x = x + 1\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(!er_has_errors(), "Variable referencing itself with definition "
                              "before on same line shouldn't generate errors");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, variable_used_before_definition_different_lines) {
  const char *program = "PRINT x\n"
                        "LET x = 5\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Variable used before definition should generate error");
  cr_assert_eq(er_get_error_count(), 1, "Should have exactly one error");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, complex_use_before_definition) {
  const char *program = "LET y = x * 2\n"
                        "LET x = 10\n"
                        "PRINT y\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Complex use before definition should generate error");
  cr_assert_eq(er_get_error_count(), 1, "Should have exactly one error for x");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, variable_in_control_flow_before_definition) {
  const char *program = "IF x > 0 THEN\n"
                        "  PRINT \"positive\"\n"
                        "ENDIF\n"
                        "LET x = 5\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Variable in control flow before definition should generate error");
  cr_assert_eq(er_get_error_count(), 1, "Should have exactly one error");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, multiple_variables_order_dependency) {
  const char *program = "LET a = b + 1\n"
                        "LET b = c * 2\n"
                        "LET c = 5\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Variables with order dependency should generate errors");
  cr_assert_eq(er_get_error_count(), 2,
               "Should have two errors: b used before definition, c used "
               "before definition");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

// =========================
// UNKNOWN LABEL TESTS
// =========================

Test(SemanticAnalyzer, goto_unknown_label) {
  const char *program = "GOTO unknown\n"
                        "PRINT \"test\"\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(), "GOTO to unknown label should generate error");
  cr_assert_eq(er_get_error_count(), 1, "Should have exactly one error");

  CompilerError error = er_get_error_at(0);
  cr_assert_eq(error.type, ERROR_SEMANTIC, "Error should be semantic type");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, multiple_gotos_unknown_labels) {
  const char *program = "GOTO first\n"
                        "GOTO second\n"
                        "PRINT \"test\"\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Multiple GOTOs to unknown labels should generate errors");
  cr_assert_eq(er_get_error_count(), 2, "Should have two errors");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, goto_to_variable_name) {
  const char *program = "LET x = 5\n"
                        "GOTO x\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(), "GOTO to variable name should generate error");
  cr_assert_eq(er_get_error_count(), 1, "Should have exactly one error");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, forward_reference_label) {
  const char *program = "GOTO end\n"
                        "PRINT \"middle\"\n"
                        "LABEL end\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Forward reference to label should be valid");
  cr_assert(!er_has_errors(),
            "Forward reference to label should not generate error");

  cleanup_test_data(&ast, &ta, table);
}

Test(SemanticAnalyzer, backward_reference_label) {
  const char *program = "LABEL start\n"
                        "PRINT \"test\"\n"
                        "GOTO start\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Backward reference to label should be valid");
  cr_assert(!er_has_errors(),
            "Backward reference to label should not generate error");

  cleanup_test_data(&ast, &ta, table);
}

// =========================
// DUPLICATE LABEL TESTS
// =========================

Test(SemanticAnalyzer, duplicate_labels_same_name) {
  const char *program = "LABEL start\n"
                        "PRINT \"first\"\n"
                        "LABEL start\n"
                        "PRINT \"second\"\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(), "Duplicate labels should generate error");
  cr_assert_eq(er_get_error_count(), 1, "Should have exactly one error");

  CompilerError error = er_get_error_at(0);
  cr_assert_eq(error.type, ERROR_SEMANTIC, "Error should be semantic type");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, multiple_duplicate_labels) {
  const char *program = "LABEL loop\n"
                        "PRINT \"first loop\"\n"
                        "LABEL end\n"
                        "PRINT \"first end\"\n"
                        "LABEL loop\n"
                        "PRINT \"second loop\"\n"
                        "LABEL end\n"
                        "PRINT \"second end\"\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Multiple duplicate labels should generate errors");
  cr_assert_eq(er_get_error_count(), 2,
               "Should have two errors for duplicate labels");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, duplicate_label_in_control_flow) {
  const char *program = "LABEL start\n"
                        "IF x > 0 THEN\n"
                        "  LABEL start\n"
                        "  PRINT \"inside if\"\n"
                        "ENDIF\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Duplicate label in control flow should generate error");
  cr_assert_eq(er_get_error_count(), 2,
               "Should have two errors: undefined x and duplicate label");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, label_and_variable_same_name) {
  const char *program = "LET loop = 5\n"
                        "LABEL loop\n"
                        "PRINT loop\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Labels and variables can have same name");
  cr_assert(!er_has_errors(),
            "Label and variable with same name should not generate error");

  cleanup_test_data(&ast, &ta, table);
}

// =========================
// COMPLEX MIXED ERROR SCENARIOS
// =========================

Test(SemanticAnalyzer, mixed_errors_all_types) {
  const char *program = "GOTO nonexistent\n"    // Unknown label
                        "PRINT undefined_var\n" // Undefined variable
                        "LET y = z + 1\n" // Variable used before definition
                        "LABEL duplicate\n"
                        "LABEL duplicate\n" // Duplicate label
                        "LET z = 10\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Program with mixed errors should generate errors");
  cr_assert_eq(er_get_error_count(), 4,
               "Should have four errors: unknown label, undefined var, use "
               "before def, duplicate label");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, complex_control_flow_with_errors) {
  const char *program = "IF undefined_condition > 0 THEN\n"
                        "  LABEL inside_if\n"
                        "  PRINT used_before_def\n"
                        "  WHILE another_undefined < 10 REPEAT\n"
                        "    LABEL inside_if\n" // Duplicate
                        "    LET used_before_def = 5\n"
                        "  ENDWHILE\n"
                        "  GOTO missing_label\n"
                        "ENDIF\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Complex control flow with errors should generate multiple errors");
  cr_assert(er_get_error_count() > 3, "Should have multiple errors");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

Test(SemanticAnalyzer, edge_case_same_variable_multiple_uses_before_def) {
  const char *program = "PRINT x\n"
                        "LET y = x + x * x\n"
                        "IF x > 0 THEN\n"
                        "  PRINT x\n"
                        "ENDIF\n"
                        "LET x = 42\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Function should return true even with semantic errors");
  cr_assert(er_has_errors(),
            "Multiple uses of same undefined variable should generate errors");
  cr_assert_eq(er_get_error_count(), 6,
               "Should have six errors for each x usage before definition");

  cleanup_test_data(&ast, &ta, table);
  er_free();
}

// =========================
// EDGE CASES AND BOUNDARY CONDITIONS
// =========================

Test(SemanticAnalyzer, empty_label_name_edge_case) {
  // This tests what happens if the AST structure is malformed
  const char *program = "LET x = 5\n"
                        "PRINT x\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Normal program should pass");
  cr_assert(!er_has_errors(), "Normal program should have no errors");

  cleanup_test_data(&ast, &ta, table);
}

Test(SemanticAnalyzer, very_long_identifier_names) {
  const char *program =
      "LET very_long_variable_name_that_exceeds_normal_length = 123\n"
      "LABEL very_long_label_name_that_exceeds_normal_length\n"
      "PRINT very_long_variable_name_that_exceeds_normal_length\n"
      "GOTO very_long_label_name_that_exceeds_normal_length\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Long identifier names should work");
  cr_assert(!er_has_errors(),
            "Long identifier names should not generate errors");

  cleanup_test_data(&ast, &ta, table);
}

Test(SemanticAnalyzer, case_sensitive_identifiers) {
  const char *program = "LET Variable = 5\n"
                        "LET variable = 10\n"
                        "LET VARIABLE = 15\n"
                        "PRINT Variable\n"
                        "PRINT variable\n"
                        "PRINT VARIABLE\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Case-sensitive identifiers should work");
  cr_assert(!er_has_errors(),
            "Case-sensitive identifiers should not generate errors");

  cleanup_test_data(&ast, &ta, table);
}

Test(SemanticAnalyzer, deeply_nested_scopes) {
  const char *program = "LET outer = 1\n"
                        "IF outer > 0 THEN\n"
                        "  LET middle = 2\n"
                        "  WHILE middle < 5 REPEAT\n"
                        "    IF outer < middle THEN\n"
                        "      LET inner = 3\n"
                        "      PRINT inner\n"
                        "      PRINT middle\n"
                        "      PRINT outer\n"
                        "    ENDIF\n"
                        "    LET middle = middle + 1\n"
                        "  ENDWHILE\n"
                        "ENDIF\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Deeply nested scopes should work");
  cr_assert(!er_has_errors(),
            "Deeply nested scopes should not generate errors");

  cleanup_test_data(&ast, &ta, table);
}

Test(SemanticAnalyzer, numeric_expressions_complex) {
  const char *program = "LET a = 1\n"
                        "LET b = 2\n"
                        "LET c = 3\n"
                        "LET result = a + b * c - a / b + c * a\n"
                        "PRINT result\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Complex numeric expressions should work");
  cr_assert(!er_has_errors(),
            "Complex numeric expressions should not generate errors");

  cleanup_test_data(&ast, &ta, table);
}

// =========================
// BOUNDARY CASES FOR POSITION TRACKING
// =========================

Test(SemanticAnalyzer, variable_definition_and_use_same_column) {
  // This tests edge cases around file position comparison
  const char *program = "LET x = 5\n"
                        "LET y = x\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true,
            "Variable definition and use should work correctly");
  cr_assert(!er_has_errors(), "Proper usage should not generate errors");

  cleanup_test_data(&ast, &ta, table);
}

Test(SemanticAnalyzer, stress_test_many_variables) {
  const char *program = "LET a = 1\n"
                        "LET b = 2\n"
                        "LET c = 3\n"
                        "LET d = 4\n"
                        "LET e = 5\n"
                        "LET f = a + b + c + d + e\n"
                        "PRINT f\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Many variables should work");
  cr_assert(!er_has_errors(), "Many variables should not generate errors");

  cleanup_test_data(&ast, &ta, table);
}

Test(SemanticAnalyzer, stress_test_many_labels) {
  const char *program = "LABEL label1\n"
                        "LABEL label2\n"
                        "LABEL label3\n"
                        "LABEL label4\n"
                        "LABEL label5\n"
                        "GOTO label1\n"
                        "GOTO label2\n"
                        "GOTO label3\n"
                        "GOTO label4\n"
                        "GOTO label5\n";

  AST ast;
  TokenArray ta = NULL;
  NameTable *table;
  setup_test_data(program, &ast, &ta, &table);

  bool result = semantic_analyzer_check(&ast, table);

  cr_assert(result == true, "Many labels should work");
  cr_assert(!er_has_errors(), "Many labels should not generate errors");

  cleanup_test_data(&ast, &ta, table);
}
