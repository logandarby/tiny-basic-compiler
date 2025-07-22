#include "../src/ast/ast.h"
#include "../src/ast/ast_utils.h"
#include <criterion/criterion.h>
#include <criterion/redirect.h>

Test(parser, test_verify_structure) {
  // Create MOCK AST representing the following program:
  // LET x = 10 + 20
  // PRINT x

  AST ast = ast_init();

  // Create root PROGRAM node
  NodeID program = ast_create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  // Statement 1: LET x = 10 + 20
  NodeID let_stmt =
      ast_node_add_child_grammar(&ast, program, GRAMMAR_TYPE_STATEMENT);

  // LET statement tokens and expression
  ast_node_add_child_token(&ast, let_stmt, token_create_simple(TOKEN_LET));
  Token ident_x = {TOKEN_IDENT, NULL}; // Use NULL for text in mock
  ast_node_add_child_token(&ast, let_stmt, ident_x);
  ast_node_add_child_token(&ast, let_stmt, token_create_simple(TOKEN_EQ));

  // Expression: 10 + 20
  NodeID expr1 =
      ast_node_add_child_grammar(&ast, let_stmt, GRAMMAR_TYPE_EXPRESSION);

  // Term: 10
  NodeID term1 = ast_node_add_child_grammar(&ast, expr1, GRAMMAR_TYPE_TERM);
  NodeID unary1 = ast_node_add_child_grammar(&ast, term1, GRAMMAR_TYPE_UNARY);
  NodeID primary1 =
      ast_node_add_child_grammar(&ast, unary1, GRAMMAR_TYPE_PRIMARY);
  Token num_10 = {TOKEN_NUMBER, NULL}; // Use NULL for text in mock
  ast_node_add_child_token(&ast, primary1, num_10);

  // + operator
  ast_node_add_child_token(&ast, expr1, token_create_simple(TOKEN_PLUS));

  // Term: 20
  NodeID term2 = ast_node_add_child_grammar(&ast, expr1, GRAMMAR_TYPE_TERM);
  NodeID unary2 = ast_node_add_child_grammar(&ast, term2, GRAMMAR_TYPE_UNARY);
  NodeID primary2 =
      ast_node_add_child_grammar(&ast, unary2, GRAMMAR_TYPE_PRIMARY);
  Token num_20 = {TOKEN_NUMBER, NULL}; // Use NULL for text in mock
  ast_node_add_child_token(&ast, primary2, num_20);

  // Statement 2: PRINT x
  NodeID print_stmt =
      ast_node_add_child_grammar(&ast, program, GRAMMAR_TYPE_STATEMENT);

  ast_node_add_child_token(&ast, print_stmt, token_create_simple(TOKEN_PRINT));

  // Expression: x
  NodeID print_expr =
      ast_node_add_child_grammar(&ast, print_stmt, GRAMMAR_TYPE_EXPRESSION);
  NodeID print_term =
      ast_node_add_child_grammar(&ast, print_expr, GRAMMAR_TYPE_TERM);
  NodeID print_unary =
      ast_node_add_child_grammar(&ast, print_term, GRAMMAR_TYPE_UNARY);
  NodeID print_primary =
      ast_node_add_child_grammar(&ast, print_unary, GRAMMAR_TYPE_PRIMARY);
  Token ident_x_print = {TOKEN_IDENT, NULL}; // Use NULL for text in mock
  ast_node_add_child_token(&ast, print_primary, ident_x_print);

  // Verify the structure
  ast_verify_structure(
      &ast, "PROGRAM(STATEMENT(LET,IDENT,EQ,EXPRESSION("
            "TERM(UNARY(PRIMARY(NUMBER))),PLUS,TERM(UNARY(PRIMARY(NUMBER))))),"
            "STATEMENT(PRINT,EXPRESSION(TERM(UNARY(PRIMARY(IDENT))))))");

  ast_destroy(&ast);
}
