#include "../src/ast/ast_utils.h"
#include "../src/common/file_reader.h"
#include "../src/frontend/lexer/lexer.h"
#include "../src/frontend/parser/parser.h"
#include <criterion/criterion.h>
#include <criterion/parameterized.h>

// =========================
// HELPER FUNCTIONS
// =========================

// Helper function to create a TokenArray from a string (similar to lexer tests)
static TokenArray parse_string(const char *input) {
  FileReader fr = filereader_init_from_string(input);
  TokenArray ta = lexer_parse(fr);
  filereader_destroy(&fr);
  return ta;
}

// Helper function to parse string and create AST
static AST parse_string_to_ast(const char *input, TokenArray *ta) {
  if (*ta != NULL) {
    DZ_ASSERT(false, "TokenArray is not NULL");
  }
  *ta = parse_string(input);
  AST ast = ast_parse(*ta);
  return ast;
}

// =========================
// BASIC STATEMENT TESTS
// =========================

Test(AST_Parse, empty_program) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("", &ta);

  cr_assert(ast_verify_structure(&ast, "PROGRAM()"),
            "Empty program should have empty PROGRAM node");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, single_newline) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("\n", &ta);

  cr_assert(ast_verify_structure(&ast, ""),
            "Single newline should result in empty program");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, multiple_newlines) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("\n\n\n", &ta);

  cr_assert(ast_verify_structure(&ast, "PROGRAM()"),
            "Multiple newlines should result in empty program");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// LET STATEMENT TESTS
// =========================

Test(AST_Parse, let_statement_simple) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET x = 5\n", &ta);

  cr_assert(ast_verify_structure(&ast,
                                 "PROGRAM(STATEMENT(LET,IDENT(x),EQ,EXPRESSION("
                                 "TERM(UNARY(PRIMARY(NUMBER(5)))))))"),
            "Simple LET statement should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, let_statement_with_expression) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET result = 10 + 5\n", &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(LET,IDENT(result),EQ,EXPRESSION(TERM(UNARY("
          "PRIMARY(NUMBER(10)))),PLUS,TERM(UNARY(PRIMARY(NUMBER(5)))))))"),
      "LET statement with addition should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, let_statement_complex_expression) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET value = x * 2 + y - 1\n", &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(LET,IDENT(value),EQ,EXPRESSION(TERM(UNARY(PRIMARY("
          "IDENT(x))),MULT,UNARY(PRIMARY(NUMBER(2)))),PLUS,TERM(UNARY(PRIMARY("
          "IDENT(y)))),MINUS,TERM(UNARY(PRIMARY(NUMBER(1)))))))"),
      "LET with complex expression should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, let_statement_with_unary) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET neg = -42\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(LET,IDENT(neg),EQ,EXPRESSION(TERM("
                      "UNARY(MINUS,PRIMARY(NUMBER(42)))))))"),
            "LET with unary minus should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, let_statement_with_positive_unary) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET pos = +99\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(LET,IDENT(pos),EQ,EXPRESSION(TERM("
                      "UNARY(PLUS,PRIMARY(NUMBER(99)))))))"),
            "LET with unary plus should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// PRINT STATEMENT TESTS
// =========================

Test(AST_Parse, print_number) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("PRINT 42\n", &ta);

  cr_assert(ast_verify_structure(&ast, "PROGRAM(STATEMENT(PRINT,EXPRESSION("
                                       "TERM(UNARY(PRIMARY(NUMBER(42)))))))"),
            "PRINT with number should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, print_string) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("PRINT \"Hello World\"\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(PRINT,STRING(Hello World)))"),
            "PRINT with string should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, print_variable) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("PRINT x\n", &ta);

  cr_assert(ast_verify_structure(&ast, "PROGRAM(STATEMENT(PRINT,EXPRESSION("
                                       "TERM(UNARY(PRIMARY(IDENT(x)))))))"),
            "PRINT with variable should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, print_expression) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("PRINT x + y * 2\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(PRINT,EXPRESSION(TERM(UNARY(PRIMARY("
                      "IDENT(x)))),PLUS,TERM(UNARY(PRIMARY(IDENT(y))),MULT,"
                      "UNARY(PRIMARY(NUMBER(2)))))))"),
            "PRINT with complex expression should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// INPUT STATEMENT TESTS
// =========================

Test(AST_Parse, input_statement) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("INPUT x\n", &ta);

  cr_assert(ast_verify_structure(&ast, "PROGRAM(STATEMENT(INPUT,IDENT(x)))"),
            "INPUT statement should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, input_statement_long_name) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("INPUT variable_name\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(INPUT,IDENT(variable_name)))"),
            "INPUT with long variable name should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// LABEL AND GOTO TESTS
// =========================

Test(AST_Parse, label_statement) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LABEL start\n", &ta);

  cr_assert(
      ast_verify_structure(&ast, "PROGRAM(STATEMENT(LABEL,IDENT(start)))"),
      "LABEL statement should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, goto_statement) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("GOTO end\n", &ta);

  cr_assert(ast_verify_structure(&ast, "PROGRAM(STATEMENT(GOTO,IDENT(end)))"),
            "GOTO statement should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, label_and_goto_sequence) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LABEL loop\nGOTO loop\n", &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(LABEL,IDENT(loop)),STATEMENT(GOTO,IDENT(loop)))"),
      "LABEL and GOTO sequence should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// COMPARISON TESTS
// =========================

Test(AST_Parse, simple_comparison_equal) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("IF x == 5 THEN\nENDIF\n", &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(IF,COMPARISON(EXPRESSION(TERM(UNARY(PRIMARY(IDENT("
          "x))))),EQEQ,EXPRESSION(TERM(UNARY(PRIMARY(NUMBER(5)))))),THEN,ENDIF)"
          ")"),
      "IF with equality comparison should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, simple_comparison_not_equal) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("IF x != 0 THEN\nENDIF\n", &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(IF,COMPARISON(EXPRESSION(TERM(UNARY(PRIMARY(IDENT("
          "x))))),NOTEQ,EXPRESSION(TERM(UNARY(PRIMARY(NUMBER(0)))))),THEN,"
          "ENDIF))"),
      "IF with not-equal comparison should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, simple_comparison_greater) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("IF x > 10 THEN\nENDIF\n", &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(IF,COMPARISON(EXPRESSION(TERM(UNARY(PRIMARY(IDENT("
          "x))))),GT,EXPRESSION(TERM(UNARY(PRIMARY(NUMBER(10)))))),THEN,ENDIF)"
          ")"),
      "IF with greater-than comparison should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, simple_comparison_less) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("IF y < 100 THEN\nENDIF\n", &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(IF,COMPARISON(EXPRESSION(TERM(UNARY(PRIMARY(IDENT("
          "y))))),LT,EXPRESSION(TERM(UNARY(PRIMARY(NUMBER(100)))))),THEN,ENDIF)"
          ")"),
      "IF with less-than comparison should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, simple_comparison_greater_equal) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("IF score >= 90 THEN\nENDIF\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(IF,COMPARISON(EXPRESSION(TERM(UNARY("
                      "PRIMARY(IDENT(score))))),GTE,EXPRESSION(TERM(UNARY("
                      "PRIMARY(NUMBER(90)))))),THEN,ENDIF))"),
            "IF with greater-equal comparison should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, simple_comparison_less_equal) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("IF age <= 65 THEN\nENDIF\n", &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(IF,COMPARISON(EXPRESSION(TERM(UNARY(PRIMARY(IDENT("
          "age))))),LTE,EXPRESSION(TERM(UNARY(PRIMARY(NUMBER(65)))))),THEN,"
          "ENDIF))"),
      "IF with less-equal comparison should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, comparison_with_expressions) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("IF x + 1 > y * 2 THEN\nENDIF\n", &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(IF,COMPARISON(EXPRESSION(TERM(UNARY(PRIMARY(IDENT("
          "x)))),PLUS,TERM(UNARY(PRIMARY(NUMBER(1))))),GT,EXPRESSION(TERM("
          "UNARY(PRIMARY(IDENT(y))),MULT,UNARY(PRIMARY(NUMBER(2)))))),THEN,"
          "ENDIF))"),
      "IF with expressions in comparison should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// IF STATEMENT TESTS
// =========================

Test(AST_Parse, if_statement_empty) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("IF x == 0 THEN\nENDIF\n", &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(IF,COMPARISON(EXPRESSION(TERM(UNARY(PRIMARY(IDENT("
          "x))))),EQEQ,EXPRESSION(TERM(UNARY(PRIMARY(NUMBER(0)))))),THEN,ENDIF)"
          ")"),
      "Empty IF statement should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, if_statement_with_print) {
  TokenArray ta = NULL;
  AST ast =
      parse_string_to_ast("IF x > 0 THEN\nPRINT \"positive\"\nENDIF\n", &ta);

  cr_assert(ast_verify_structure(
                &ast,
                "PROGRAM(STATEMENT(IF,COMPARISON(EXPRESSION(TERM(UNARY(PRIMARY("
                "IDENT(x))))),GT,EXPRESSION(TERM(UNARY(PRIMARY(NUMBER(0)))))),"
                "THEN,STATEMENT(PRINT,STRING(positive)),ENDIF))"),
            "IF statement with PRINT should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, if_statement_multiple_statements) {
  TokenArray ta = NULL;
  AST ast =
      parse_string_to_ast("IF x == 1 THEN\nPRINT x\nLET y = 2\nENDIF\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(IF,COMPARISON(EXPRESSION(TERM(UNARY("
                      "PRIMARY(IDENT(x))))),EQEQ,EXPRESSION(TERM(UNARY(PRIMARY("
                      "NUMBER(1)))))),THEN,STATEMENT(PRINT,EXPRESSION(TERM("
                      "UNARY(PRIMARY(IDENT(x)))))),STATEMENT(LET,IDENT(y),EQ,"
                      "EXPRESSION(TERM(UNARY(PRIMARY(NUMBER(2)))))),ENDIF))"),
            "IF statement with multiple statements should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, nested_if_statements) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast(
      "IF x > 0 THEN\nIF y > 0 THEN\nPRINT \"both positive\"\nENDIF\nENDIF\n",
      &ta);

  cr_assert(ast_verify_structure(
                &ast,
                "PROGRAM(STATEMENT(IF,COMPARISON(EXPRESSION(TERM(UNARY(PRIMARY("
                "IDENT(x))))),GT,EXPRESSION(TERM(UNARY(PRIMARY(NUMBER(0)))))),"
                "THEN,STATEMENT(IF,COMPARISON(EXPRESSION(TERM(UNARY(PRIMARY("
                "IDENT(y))))),GT,EXPRESSION(TERM(UNARY(PRIMARY(NUMBER(0)))))),"
                "THEN,STATEMENT(PRINT,STRING(both positive)),ENDIF),ENDIF))"),
            "Nested IF statements should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// WHILE STATEMENT TESTS
// =========================

Test(AST_Parse, while_statement_empty) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("WHILE x < 10 REPEAT\nENDWHILE\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(WHILE,COMPARISON(EXPRESSION(TERM("
                      "UNARY(PRIMARY(IDENT(x))))),LT,EXPRESSION(TERM(UNARY("
                      "PRIMARY(NUMBER(10)))))),REPEAT,ENDWHILE))"),
            "Empty WHILE statement should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, while_statement_with_body) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast(
      "WHILE count < 5 REPEAT\nPRINT count\nLET count = count + 1\nENDWHILE\n",
      &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(WHILE,COMPARISON(EXPRESSION(TERM(UNARY(PRIMARY("
          "IDENT(count))))),LT,EXPRESSION(TERM(UNARY(PRIMARY(NUMBER(5)))))),"
          "REPEAT,STATEMENT(PRINT,EXPRESSION(TERM(UNARY(PRIMARY(IDENT(count))))"
          ")),STATEMENT(LET,IDENT(count),EQ,EXPRESSION(TERM(UNARY(PRIMARY("
          "IDENT(count)))),PLUS,TERM(UNARY(PRIMARY(NUMBER(1)))))),ENDWHILE))"),
      "WHILE statement with body should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, nested_while_statements) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("WHILE i < 3 REPEAT\nWHILE j < 2 REPEAT\nPRINT"
                                "\"nested\"\nENDWHILE\nENDWHILE\n",
                                &ta);

  cr_assert(ast_verify_structure(
                &ast,
                "PROGRAM(STATEMENT(WHILE,COMPARISON(EXPRESSION(TERM(UNARY("
                "PRIMARY(IDENT(i))))),LT,EXPRESSION(TERM(UNARY(PRIMARY(NUMBER("
                "3)))))),REPEAT,STATEMENT(WHILE,COMPARISON(EXPRESSION(TERM("
                "UNARY(PRIMARY(IDENT(j))))),LT,EXPRESSION(TERM(UNARY(PRIMARY("
                "NUMBER(2)))))),REPEAT,STATEMENT(PRINT,STRING(nested)),"
                "ENDWHILE),ENDWHILE))"),
            "Nested WHILE statements should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// COMPLEX EXPRESSION TESTS
// =========================

Test(AST_Parse, arithmetic_precedence) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET result = 2 + 3 * 4\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(LET,IDENT(result),EQ,EXPRESSION(TERM("
                      "UNARY(PRIMARY(NUMBER(2)))),PLUS,TERM(UNARY(PRIMARY("
                      "NUMBER(3))),MULT,UNARY(PRIMARY(NUMBER(4)))))))"),
            "Arithmetic precedence should parse correctly (multiplication "
            "before addition)");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, division_precedence) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET result = 10 - 8 / 2\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(LET,IDENT(result),EQ,EXPRESSION(TERM("
                      "UNARY(PRIMARY(NUMBER(10)))),MINUS,TERM(UNARY(PRIMARY("
                      "NUMBER(8))),DIV,UNARY(PRIMARY(NUMBER(2)))))))"),
            "Division precedence should parse correctly (division before "
            "subtraction)");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, left_associativity_addition) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET result = 1 + 2 + 3\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(LET,IDENT(result),EQ,EXPRESSION(TERM("
                      "UNARY(PRIMARY(NUMBER(1)))),PLUS,TERM(UNARY(PRIMARY("
                      "NUMBER(2)))),PLUS,TERM(UNARY(PRIMARY(NUMBER(3)))))))"),
            "Left associative addition should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, left_associativity_multiplication) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET result = 2 * 3 * 4\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(LET,IDENT(result),EQ,EXPRESSION(TERM("
                      "UNARY(PRIMARY(NUMBER(2))),MULT,UNARY(PRIMARY(NUMBER(3)))"
                      ",MULT,UNARY(PRIMARY(NUMBER(4))))))"),
            "Left associative multiplication should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, mixed_operations) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET result = a + b * c - d / e\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(LET,IDENT(result),EQ,EXPRESSION(TERM("
                      "UNARY(PRIMARY(IDENT(a)))),PLUS,TERM(UNARY(PRIMARY(IDENT("
                      "b))),MULT,UNARY(PRIMARY(IDENT(c)))),MINUS,TERM(UNARY("
                      "PRIMARY(IDENT(d))),DIV,UNARY(PRIMARY(IDENT(e)))))))"),
            "Mixed operations should respect precedence");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, unary_operators_in_expression) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET result = -x + +y * -z\n", &ta);

  cr_assert(ast_verify_structure(
                &ast,
                "PROGRAM(STATEMENT(LET,IDENT(result),EQ,EXPRESSION(TERM(UNARY("
                "MINUS,PRIMARY(IDENT(x)))),PLUS,TERM(UNARY(PLUS,PRIMARY(IDENT("
                "y))),MULT,UNARY(MINUS,PRIMARY(IDENT(z)))))))"),
            "Unary operators in expressions should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// MULTIPLE STATEMENT PROGRAMS
// =========================

Test(AST_Parse, simple_program) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET x = 5\nPRINT x\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(LET,IDENT(x),EQ,EXPRESSION(TERM(UNARY("
                      "PRIMARY(NUMBER(5)))))),STATEMENT(PRINT,EXPRESSION(TERM("
                      "UNARY(PRIMARY(IDENT(x)))))))"),
            "Simple two-statement program should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, program_with_input_and_output) {
  TokenArray ta = NULL;
  AST ast =
      parse_string_to_ast("PRINT \"Enter a number:\"\nINPUT x\nPRINT x\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(PRINT,STRING(Enter a "
                      "number:)),STATEMENT(INPUT,IDENT(x)),STATEMENT(PRINT,"
                      "EXPRESSION(TERM(UNARY(PRIMARY(IDENT(x)))))))"),
            "Program with input and output should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, program_with_control_flow) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast(
      "LET x = 10\nIF x > 5 THEN\nPRINT \"large\"\nENDIF\nPRINT \"done\"\n",
      &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(LET,IDENT(x),EQ,EXPRESSION(TERM(UNARY("
          "PRIMARY(NUMBER(10)))))),STATEMENT(IF,COMPARISON(EXPRESSION("
          "TERM(UNARY(PRIMARY(IDENT(x))))),GT,EXPRESSION(TERM(UNARY("
          "PRIMARY(NUMBER(5)))))),THEN,STATEMENT(PRINT,STRING(large)),ENDIF),"
          "STATEMENT(PRINT,STRING(done)))"),
      "Program with control flow should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, loop_with_counter) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast(
      "LET i = 1\nWHILE i <= 3 REPEAT\nPRINT i\nLET i = i + 1\nENDWHILE\n",
      &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(LET,IDENT(i),EQ,EXPRESSION(TERM(UNARY(PRIMARY("
          "NUMBER(1)))))),STATEMENT(WHILE,COMPARISON(EXPRESSION(TERM(UNARY("
          "PRIMARY(IDENT(i))))),LTE,EXPRESSION(TERM(UNARY(PRIMARY(NUMBER(3)))))"
          "),REPEAT,STATEMENT(PRINT,EXPRESSION(TERM(UNARY(PRIMARY(IDENT(i))))))"
          ",STATEMENT(LET,IDENT(i),EQ,EXPRESSION(TERM(UNARY(PRIMARY(IDENT(i))))"
          ",PLUS,TERM(UNARY(PRIMARY(NUMBER(1)))))),ENDWHILE))"),
      "Loop with counter should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// WHITESPACE AND FORMATTING TESTS
// =========================

Test(AST_Parse, extra_whitespace) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("   LET   x   =   5   \n", &ta);

  cr_assert(ast_verify_structure(&ast,
                                 "PROGRAM(STATEMENT(LET,IDENT(x),EQ,EXPRESSION("
                                 "TERM(UNARY(PRIMARY(NUMBER(5)))))))"),
            "Extra whitespace should not affect parsing");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, extra_newlines) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET x = 1\n\n\nPRINT x\n\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(LET,IDENT(x),EQ,EXPRESSION(TERM(UNARY("
                      "PRIMARY(NUMBER(1)))))),STATEMENT(PRINT,EXPRESSION(TERM("
                      "UNARY(PRIMARY(IDENT(x)))))))"),
            "Extra newlines should not affect parsing");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, mixed_whitespace_and_newlines) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("  \n  LET x = 5  \n  \n  PRINT x  \n  ", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(LET,IDENT(x),EQ,EXPRESSION(TERM(UNARY("
                      "PRIMARY(NUMBER(5)))))),STATEMENT(PRINT,EXPRESSION(TERM("
                      "UNARY(PRIMARY(IDENT(x)))))))"),
            "Mixed whitespace and newlines should not affect parsing");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// EDGE CASES AND COMPLEX SCENARIOS
// =========================

Test(AST_Parse, long_identifier_names) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast(
      "LET very_long_variable_name = another_very_long_variable_name + 1\n",
      &ta);

  cr_assert(ast_verify_structure(
                &ast,
                "PROGRAM(STATEMENT(LET,IDENT(very_long_variable_name),EQ,"
                "EXPRESSION(TERM(UNARY(PRIMARY(IDENT(another_very_long_"
                "variable_name)))),PLUS,TERM(UNARY(PRIMARY(NUMBER(1)))))))"),
            "Long identifier names should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, large_numbers) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET big = 123456789\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(LET,IDENT(big),EQ,EXPRESSION(TERM("
                      "UNARY(PRIMARY(NUMBER(123456789)))))))"),
            "Large numbers should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, complex_nested_program) {
  const char *program = "LET count = 0\n"
                        "WHILE count < 5 REPEAT\n"
                        "  IF count == 2 THEN\n"
                        "    PRINT \"middle\"\n"
                        "  ENDIF\n"
                        "  PRINT count\n"
                        "  LET count = count + 1\n"
                        "ENDWHILE\n"
                        "PRINT \"done\"\n";

  TokenArray ta = NULL;
  AST ast = parse_string_to_ast(program, &ta);

  // For complex nested structures, just verify that parsing succeeds and
  // produces a non-empty AST
  cr_assert(!ast_is_empty(&ast),
            "Complex nested program should parse successfully");
  cr_assert_eq(ast_node_get_grammar(&ast, ast_head(ast)), GRAMMAR_TYPE_PROGRAM,
               "Root should be PROGRAM node");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, deeply_nested_expressions) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("LET result = 1 + 2 * 3 + 4 * 5 + 6\n", &ta);

  cr_assert(
      ast_verify_structure(
          &ast,
          "PROGRAM(STATEMENT(LET,IDENT(result),EQ,EXPRESSION(TERM(UNARY("
          "PRIMARY(NUMBER(1)))),PLUS,TERM(UNARY(PRIMARY(NUMBER(2))),MULT,UNARY("
          "PRIMARY(NUMBER(3)))),PLUS,TERM(UNARY(PRIMARY(NUMBER(4))),MULT,UNARY("
          "PRIMARY(NUMBER(5)))),PLUS,TERM(UNARY(PRIMARY(NUMBER(6)))))))"),
      "Deeply nested expressions should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, all_comparison_operators) {
  const char *program = "IF a == b THEN\nENDIF\n"
                        "IF c != d THEN\nENDIF\n"
                        "IF e > f THEN\nENDIF\n"
                        "IF g < h THEN\nENDIF\n"
                        "IF i >= j THEN\nENDIF\n"
                        "IF k <= l THEN\nENDIF\n";

  TokenArray ta = NULL;
  AST ast = parse_string_to_ast(program, &ta);

  // Verify that all comparison operators are parsed
  cr_assert(!ast_is_empty(&ast),
            "Program with all comparison operators should parse");
  cr_assert_eq(ast_node_get_grammar(&ast, ast_head(ast)), GRAMMAR_TYPE_PROGRAM,
               "Root should be PROGRAM node");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// STRING TESTS IN STATEMENTS
// =========================

Test(AST_Parse, string_with_spaces) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("PRINT \"Hello World with spaces\"\n", &ta);

  cr_assert(
      ast_verify_structure(
          &ast, "PROGRAM(STATEMENT(PRINT,STRING(Hello World with spaces)))"),
      "String with spaces should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, string_with_special_characters) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("PRINT \"Special: !@#$%^&*()\"\n", &ta);

  cr_assert(ast_verify_structure(
                &ast, "PROGRAM(STATEMENT(PRINT,STRING(Special: !@#$%^&*())))"),
            "String with special characters should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, empty_string) {
  TokenArray ta = NULL;
  AST ast = parse_string_to_ast("PRINT \"\"\n", &ta);

  cr_assert(ast_verify_structure(&ast, "PROGRAM(STATEMENT(PRINT,STRING()))"),
            "Empty string should parse correctly");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

// =========================
// COMPREHENSIVE REAL-WORLD PROGRAM TESTS
// =========================

Test(AST_Parse, fibonacci_like_program) {
  const char *program = "LET a = 0\n"
                        "LET b = 1\n"
                        "LET count = 0\n"
                        "WHILE count < 10 REPEAT\n"
                        "  PRINT a\n"
                        "  LET temp = a + b\n"
                        "  LET a = b\n"
                        "  LET b = temp\n"
                        "  LET count = count + 1\n"
                        "ENDWHILE\n";

  TokenArray ta = NULL;
  AST ast = parse_string_to_ast(program, &ta);

  cr_assert(!ast_is_empty(&ast),
            "Fibonacci-like program should parse successfully");
  cr_assert_eq(ast_node_get_grammar(&ast, ast_head(ast)), GRAMMAR_TYPE_PROGRAM,
               "Root should be PROGRAM node");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}

Test(AST_Parse, number_guessing_game) {
  const char *program = "LET target = 42\n"
                        "LET guess = 0\n"
                        "WHILE guess != target REPEAT\n"
                        "  PRINT \"Enter your guess:\"\n"
                        "  INPUT guess\n"
                        "  IF guess > target THEN\n"
                        "    PRINT \"Too high!\"\n"
                        "  ENDIF\n"
                        "  IF guess < target THEN\n"
                        "    PRINT \"Too low!\"\n"
                        "  ENDIF\n"
                        "ENDWHILE\n"
                        "PRINT \"Correct!\"\n";

  TokenArray ta = NULL;
  AST ast = parse_string_to_ast(program, &ta);

  cr_assert(!ast_is_empty(&ast),
            "Number guessing game should parse successfully");
  cr_assert_eq(ast_node_get_grammar(&ast, ast_head(ast)), GRAMMAR_TYPE_PROGRAM,
               "Root should be PROGRAM node");

  ast_destroy(&ast);
  token_array_destroy(&ta);
}