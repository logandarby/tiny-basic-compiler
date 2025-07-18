#include "../src/parser.h"
#include <criterion/criterion.h>
#include <criterion/parameterized.h>
#include <stdio.h>
#include <string.h>

// =========================
// HELPER FUNCTIONS
// =========================

// Helper function to create a test token
static Token create_test_token(enum TOKEN type, const char *text) {
  Token token = {.type = type, .text = NULL};

  // For tokens that should have text, allocate and copy
  if (text != NULL) {
    size_t len = strlen(text);
    token.text = malloc(len + 1);
    strcpy(token.text, text);
  }

  return token;
}

// Helper function to clean up a test token
static void destroy_test_token(Token *token) {
  if (token && token->text) {
    free(token->text);
    token->text = NULL;
  }
}

// Helper to create a root grammar node using the public API
static NodeID create_root_node(AST *ast, GRAMMAR_TYPE grammar_type) {
  return ast_create_root_node(ast, grammar_type);
}

// =========================
// INITIALIZATION AND DESTRUCTION TESTS
// =========================

Test(parser, ast_init_creates_empty_ast) {
  AST ast = ast_init();

  cr_assert_not_null(ast.node_array, "AST node_array should not be NULL");
  cr_assert_eq(ast.node_array_size, 0, "New AST should have 0 nodes");
  cr_assert_gt(ast.node_array_capacity, 0, "AST should have positive capacity");

  ast_destroy(&ast);
}

Test(parser, ast_destroy_cleans_up_memory) {
  AST ast = ast_init();

  // Add some nodes to test cleanup
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);
  Token token = create_test_token(TOKEN_NUMBER, "42");
  ast_node_add_child_token(&ast, root, token);

  // Should not crash and should clean up properly
  ast_destroy(&ast);

  // Cleanup our test token
  destroy_test_token(&token);
}

Test(parser, ast_destroy_handles_empty_ast) {
  AST ast = ast_init();

  // Should not crash on empty AST
  ast_destroy(&ast);
}

Test(parser, ast_destroy_handles_null_array) {
  AST ast = ast_init();
  free(ast.node_array);
  ast.node_array = NULL;

  // Should not crash with NULL array
  ast_destroy(&ast);
}

Test(parser, ast_create_root_node_creates_proper_root) {
  AST ast = ast_init();

  NodeID root = ast_create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  cr_assert_eq(root, 0, "Root node should have ID 0");
  cr_assert_eq(ast.node_array_size, 1,
               "AST should have 1 node after creating root");
  cr_assert_eq(ast_head(ast), root, "Head should point to the root node");

  cr_assert(ast_node_is_grammar(&ast, root), "Root should be a grammar node");
  cr_assert(!ast_node_is_token(&ast, root), "Root should not be a token node");

  GRAMMAR_TYPE grammar_type = ast_node_get_grammar(&ast, root);
  cr_assert_eq(grammar_type, GRAMMAR_TYPE_PROGRAM,
               "Root should have correct grammar type");

  cr_assert_eq(ast_node_get_child_count(&ast, root), 0,
               "Root should have no children initially");

  ast_destroy(&ast);
}

// =========================
// HEAD NODE TESTS
// =========================

Test(parser, ast_head_returns_head_when_nodes_exist) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  NodeID head = ast_head(ast);
  cr_assert_eq(head, root, "ast_head should return the head node ID");

  ast_destroy(&ast);
}

// Note: ast_head with empty AST should assert/crash according to the DZ_ASSERT
// We can't easily test this in Criterion without fork/signal handling

// =========================
// NODE TYPE CHECKING TESTS
// =========================

Test(parser, ast_node_is_token_correctly_identifies_token_nodes) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  Token token = create_test_token(TOKEN_NUMBER, "42");
  ast_node_add_child_token(&ast, root, token);
  NodeID token_node = ast_node_get_child(&ast, root, 0);

  cr_assert(ast_node_is_token(&ast, token_node),
            "Token node should be identified as token");
  cr_assert(!ast_node_is_token(&ast, root),
            "Grammar node should not be identified as token");

  ast_destroy(&ast);
  destroy_test_token(&token);
}

Test(parser, ast_node_is_grammar_correctly_identifies_grammar_nodes) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  ast_node_add_child_grammar(&ast, root, GRAMMAR_TYPE_EXPRESSION);
  NodeID grammar_node = ast_node_get_child(&ast, root, 0);

  cr_assert(ast_node_is_grammar(&ast, root),
            "Grammar node should be identified as grammar");
  cr_assert(ast_node_is_grammar(&ast, grammar_node),
            "Child grammar node should be identified as grammar");

  ast_destroy(&ast);
}

Test(parser, ast_node_is_token_and_grammar_are_mutually_exclusive) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  Token token = create_test_token(TOKEN_IDENT, "variable");
  ast_node_add_child_token(&ast, root, token);
  ast_node_add_child_grammar(&ast, root, GRAMMAR_TYPE_EXPRESSION);

  NodeID token_node = ast_node_get_child(&ast, root, 0);
  NodeID grammar_node = ast_node_get_child(&ast, root, 1);

  // Token node
  cr_assert(ast_node_is_token(&ast, token_node), "Should be token");
  cr_assert(!ast_node_is_grammar(&ast, token_node), "Should not be grammar");

  // Grammar node
  cr_assert(!ast_node_is_token(&ast, grammar_node), "Should not be token");
  cr_assert(ast_node_is_grammar(&ast, grammar_node), "Should be grammar");

  ast_destroy(&ast);
  destroy_test_token(&token);
}

// =========================
// ADDING CHILD NODES TESTS
// =========================

Test(parser, ast_node_add_child_token_adds_token_correctly) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  Token token = create_test_token(TOKEN_NUMBER, "123");
  ast_node_add_child_token(&ast, root, token);

  cr_assert_eq(ast_node_get_child_count(&ast, root), 1,
               "Root should have 1 child after adding token");

  NodeID child = ast_node_get_child(&ast, root, 0);
  cr_assert(ast_node_is_token(&ast, child), "Child should be a token");

  const Token *retrieved_token = ast_node_get_token(&ast, child);
  cr_assert_eq(retrieved_token->type, TOKEN_NUMBER, "Token type should match");
  cr_assert_str_eq(retrieved_token->text, "123", "Token text should match");

  ast_destroy(&ast);
  destroy_test_token(&token);
}

Test(parser, ast_node_add_child_grammar_adds_grammar_correctly) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  ast_node_add_child_grammar(&ast, root, GRAMMAR_TYPE_EXPRESSION);

  cr_assert_eq(ast_node_get_child_count(&ast, root), 1,
               "Root should have 1 child after adding grammar");

  NodeID child = ast_node_get_child(&ast, root, 0);
  cr_assert(ast_node_is_grammar(&ast, child), "Child should be a grammar node");

  GRAMMAR_TYPE grammar_type = ast_node_get_grammar(&ast, child);
  cr_assert_eq(grammar_type, GRAMMAR_TYPE_EXPRESSION,
               "Grammar type should match");

  ast_destroy(&ast);
}

Test(parser, ast_node_add_multiple_children_maintains_order) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  Token token1 = create_test_token(TOKEN_NUMBER, "1");
  Token token2 = create_test_token(TOKEN_PLUS, NULL);
  Token token3 = create_test_token(TOKEN_NUMBER, "2");

  ast_node_add_child_token(&ast, root, token1);
  ast_node_add_child_token(&ast, root, token2);
  ast_node_add_child_token(&ast, root, token3);

  cr_assert_eq(ast_node_get_child_count(&ast, root), 3,
               "Root should have 3 children");

  // Check order is maintained
  const Token *t1 = ast_node_get_token(&ast, ast_node_get_child(&ast, root, 0));
  const Token *t2 = ast_node_get_token(&ast, ast_node_get_child(&ast, root, 1));
  const Token *t3 = ast_node_get_token(&ast, ast_node_get_child(&ast, root, 2));

  cr_assert_str_eq(t1->text, "1", "First child should be '1'");
  cr_assert_eq(t2->type, TOKEN_PLUS, "Second child should be PLUS");
  cr_assert_str_eq(t3->text, "2", "Third child should be '2'");

  ast_destroy(&ast);
  destroy_test_token(&token1);
  destroy_test_token(&token2);
  destroy_test_token(&token3);
}

Test(parser, ast_node_add_maximum_children) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  // Keep track of tokens so we can destroy them after the test
  Token tokens[AST_MAX_CHILDREN];

  // Add AST_MAX_CHILDREN (5) children
  for (int i = 0; i < AST_MAX_CHILDREN; i++) {
    char text[10];
    sprintf(text, "%d", i);
    tokens[i] = create_test_token(TOKEN_NUMBER, text);
    ast_node_add_child_token(&ast, root, tokens[i]);
  }

  cr_assert_eq(ast_node_get_child_count(&ast, root), AST_MAX_CHILDREN,
               "Should have maximum children");

  // Verify all children are correct
  for (int i = 0; i < AST_MAX_CHILDREN; i++) {
    NodeID child = ast_node_get_child(&ast, root, (short)i);
    const Token *token = ast_node_get_token(&ast, child);
    char expected[10];
    sprintf(expected, "%d", i);
    cr_assert_str_eq(token->text, expected, "Child %d should have correct text",
                     i);
  }

  ast_destroy(&ast);

  // Clean up tokens after the test
  for (int i = 0; i < AST_MAX_CHILDREN; i++) {
    destroy_test_token(&tokens[i]);
  }
}

Test(parser, ast_node_add_mixed_children_types) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  Token token = create_test_token(TOKEN_IDENT, "x");
  ast_node_add_child_token(&ast, root, token);
  ast_node_add_child_grammar(&ast, root, GRAMMAR_TYPE_EXPRESSION);

  cr_assert_eq(ast_node_get_child_count(&ast, root), 2,
               "Should have 2 children of different types");

  NodeID child1 = ast_node_get_child(&ast, root, 0);
  NodeID child2 = ast_node_get_child(&ast, root, 1);

  cr_assert(ast_node_is_token(&ast, child1), "First child should be token");
  cr_assert(ast_node_is_grammar(&ast, child2),
            "Second child should be grammar");

  ast_destroy(&ast);
  destroy_test_token(&token);
}

Test(parser, ast_node_respects_max_children_constraint) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);
  Token tokens[AST_MAX_CHILDREN] = {0};

  // Add exactly AST_MAX_CHILDREN (5) children to test the per-node child limit
  for (int i = 0; i < AST_MAX_CHILDREN; i++) {
    char text[20];
    sprintf(text, "child_%d", i);
    tokens[i] = create_test_token(TOKEN_IDENT, text);
    ast_node_add_child_token(&ast, root, tokens[i]);
  }

  cr_assert_eq(ast_node_get_child_count(&ast, root), AST_MAX_CHILDREN,
               "Should have exactly AST_MAX_CHILDREN children");

  // Verify all children are correct and accessible
  for (int i = 0; i < AST_MAX_CHILDREN; i++) {
    NodeID child = ast_node_get_child(&ast, root, (short)i);
    const Token *token = ast_node_get_token(&ast, child);
    char expected[20];
    sprintf(expected, "child_%d", i);
    cr_assert_str_eq(token->text, expected, "Child %d should have correct text",
                     i);
  }

  ast_destroy(&ast);
  for (int i = 0; i < AST_MAX_CHILDREN; i++) {
    destroy_test_token(&tokens[i]);
  }
}

// =========================
// CHILD ACCESS TESTS
// =========================

Test(parser, ast_node_get_child_returns_correct_child) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  Token token1 = create_test_token(TOKEN_NUMBER, "first");
  Token token2 = create_test_token(TOKEN_NUMBER, "second");
  Token token3 = create_test_token(TOKEN_NUMBER, "third");

  ast_node_add_child_token(&ast, root, token1);
  ast_node_add_child_token(&ast, root, token2);
  ast_node_add_child_token(&ast, root, token3);

  NodeID child0 = ast_node_get_child(&ast, root, 0);
  NodeID child1 = ast_node_get_child(&ast, root, 1);
  NodeID child2 = ast_node_get_child(&ast, root, 2);

  const Token *t0 = ast_node_get_token(&ast, child0);
  const Token *t1 = ast_node_get_token(&ast, child1);
  const Token *t2 = ast_node_get_token(&ast, child2);

  cr_assert_str_eq(t0->text, "first", "Child 0 should be 'first'");
  cr_assert_str_eq(t1->text, "second", "Child 1 should be 'second'");
  cr_assert_str_eq(t2->text, "third", "Child 2 should be 'third'");

  ast_destroy(&ast);
  destroy_test_token(&token1);
  destroy_test_token(&token2);
  destroy_test_token(&token3);
}

Test(parser, ast_node_get_child_count_returns_correct_count) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  cr_assert_eq(ast_node_get_child_count(&ast, root), 0,
               "New grammar node should have 0 children");

  Token token = create_test_token(TOKEN_NUMBER, "42");
  ast_node_add_child_token(&ast, root, token);
  cr_assert_eq(ast_node_get_child_count(&ast, root), 1,
               "Should have 1 child after adding one");

  ast_node_add_child_grammar(&ast, root, GRAMMAR_TYPE_EXPRESSION);
  cr_assert_eq(ast_node_get_child_count(&ast, root), 2,
               "Should have 2 children after adding two");

  ast_destroy(&ast);
  destroy_test_token(&token);
}

Test(parser, ast_node_get_child_count_returns_zero_for_token_nodes) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  Token token = create_test_token(TOKEN_NUMBER, "42");
  ast_node_add_child_token(&ast, root, token);
  NodeID token_node = ast_node_get_child(&ast, root, 0);

  cr_assert_eq(ast_node_get_child_count(&ast, token_node), 0,
               "Token nodes should always have 0 children");

  ast_destroy(&ast);
  destroy_test_token(&token);
}

// =========================
// TOKEN ACCESS TESTS
// =========================

Test(parser, ast_node_get_token_returns_correct_token) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  Token original_token = create_test_token(TOKEN_IDENT, "variable_name");
  ast_node_add_child_token(&ast, root, original_token);
  NodeID token_node = ast_node_get_child(&ast, root, 0);

  const Token *retrieved_token = ast_node_get_token(&ast, token_node);

  cr_assert_eq(retrieved_token->type, TOKEN_IDENT, "Token type should match");
  cr_assert_str_eq(retrieved_token->text, "variable_name",
                   "Token text should match");

  ast_destroy(&ast);
  destroy_test_token(&original_token);
}

Test(parser, ast_node_get_token_handles_null_text) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  Token original_token = create_test_token(TOKEN_PLUS, NULL);
  ast_node_add_child_token(&ast, root, original_token);
  NodeID token_node = ast_node_get_child(&ast, root, 0);

  const Token *retrieved_token = ast_node_get_token(&ast, token_node);

  cr_assert_eq(retrieved_token->type, TOKEN_PLUS, "Token type should match");
  cr_assert_null(retrieved_token->text, "Token text should be NULL");

  ast_destroy(&ast);
  destroy_test_token(&original_token);
}

// =========================
// GRAMMAR ACCESS TESTS
// =========================

Test(parser, ast_node_get_grammar_returns_correct_type) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  ast_node_add_child_grammar(&ast, root, GRAMMAR_TYPE_EXPRESSION);
  ast_node_add_child_grammar(&ast, root, GRAMMAR_TYPE_TERM);

  NodeID expr_node = ast_node_get_child(&ast, root, 0);
  NodeID term_node = ast_node_get_child(&ast, root, 1);

  GRAMMAR_TYPE expr_type = ast_node_get_grammar(&ast, expr_node);
  GRAMMAR_TYPE term_type = ast_node_get_grammar(&ast, term_node);

  cr_assert_eq(expr_type, GRAMMAR_TYPE_EXPRESSION,
               "Expression type should match");
  cr_assert_eq(term_type, GRAMMAR_TYPE_TERM, "Term type should match");

  ast_destroy(&ast);
}

Test(parser, ast_node_get_grammar_root_node) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  GRAMMAR_TYPE root_type = ast_node_get_grammar(&ast, root);
  cr_assert_eq(root_type, GRAMMAR_TYPE_PROGRAM,
               "Root grammar type should match");

  ast_destroy(&ast);
}

// =========================
// TREE STRUCTURE TESTS
// =========================

Test(parser, ast_creates_proper_tree_structure) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  // Create: program -> statement -> expression -> term -> primary
  ast_node_add_child_grammar(&ast, root, GRAMMAR_TYPE_STATEMENT);
  NodeID stmt = ast_node_get_child(&ast, root, 0);

  ast_node_add_child_grammar(&ast, stmt, GRAMMAR_TYPE_EXPRESSION);
  NodeID expr = ast_node_get_child(&ast, stmt, 0);

  ast_node_add_child_grammar(&ast, expr, GRAMMAR_TYPE_TERM);
  NodeID term = ast_node_get_child(&ast, expr, 0);

  ast_node_add_child_grammar(&ast, term, GRAMMAR_TYPE_PRIMARY);
  NodeID primary = ast_node_get_child(&ast, term, 0);

  // Add a number token to primary
  Token token = create_test_token(TOKEN_NUMBER, "42");
  ast_node_add_child_token(&ast, primary, token);
  NodeID number_node = ast_node_get_child(&ast, primary, 0);

  // Verify the tree structure
  cr_assert_eq(ast_node_get_grammar(&ast, root), GRAMMAR_TYPE_PROGRAM,
               "Root should be PROGRAM");
  cr_assert_eq(ast_node_get_grammar(&ast, stmt), GRAMMAR_TYPE_STATEMENT,
               "Should be STATEMENT");
  cr_assert_eq(ast_node_get_grammar(&ast, expr), GRAMMAR_TYPE_EXPRESSION,
               "Should be EXPRESSION");
  cr_assert_eq(ast_node_get_grammar(&ast, term), GRAMMAR_TYPE_TERM,
               "Should be TERM");
  cr_assert_eq(ast_node_get_grammar(&ast, primary), GRAMMAR_TYPE_PRIMARY,
               "Should be PRIMARY");

  const Token *final_token = ast_node_get_token(&ast, number_node);
  cr_assert_eq(final_token->type, TOKEN_NUMBER, "Final token should be NUMBER");
  cr_assert_str_eq(final_token->text, "42", "Final token should be '42'");

  ast_destroy(&ast);
  destroy_test_token(&token);
}

Test(parser, ast_handles_complex_expression_tree) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_EXPRESSION);

  // Create: expression with multiple terms: a + b * c
  Token token_a = create_test_token(TOKEN_IDENT, "a");
  Token token_plus = create_test_token(TOKEN_PLUS, NULL);
  Token token_b = create_test_token(TOKEN_IDENT, "b");
  Token token_mult = create_test_token(TOKEN_MULT, NULL);
  Token token_c = create_test_token(TOKEN_IDENT, "c");

  ast_node_add_child_token(&ast, root, token_a);
  ast_node_add_child_token(&ast, root, token_plus);
  ast_node_add_child_token(&ast, root, token_b);
  ast_node_add_child_token(&ast, root, token_mult);
  ast_node_add_child_token(&ast, root, token_c);

  cr_assert_eq(ast_node_get_child_count(&ast, root), 5,
               "Expression should have 5 tokens");

  // Verify all tokens are correct
  const Token *t1 = ast_node_get_token(&ast, ast_node_get_child(&ast, root, 0));
  const Token *t2 = ast_node_get_token(&ast, ast_node_get_child(&ast, root, 1));
  const Token *t3 = ast_node_get_token(&ast, ast_node_get_child(&ast, root, 2));
  const Token *t4 = ast_node_get_token(&ast, ast_node_get_child(&ast, root, 3));
  const Token *t5 = ast_node_get_token(&ast, ast_node_get_child(&ast, root, 4));

  cr_assert_str_eq(t1->text, "a", "First token should be 'a'");
  cr_assert_eq(t2->type, TOKEN_PLUS, "Second token should be PLUS");
  cr_assert_str_eq(t3->text, "b", "Third token should be 'b'");
  cr_assert_eq(t4->type, TOKEN_MULT, "Fourth token should be MULT");
  cr_assert_str_eq(t5->text, "c", "Fifth token should be 'c'");

  ast_destroy(&ast);
  destroy_test_token(&token_a);
  destroy_test_token(&token_plus);
  destroy_test_token(&token_b);
  destroy_test_token(&token_mult);
  destroy_test_token(&token_c);
}

// =========================
// EDGE CASES AND ERROR CONDITIONS
// =========================

Test(parser, ast_handles_empty_grammar_nodes) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  // Add empty grammar nodes
  ast_node_add_child_grammar(&ast, root, GRAMMAR_TYPE_EXPRESSION);
  ast_node_add_child_grammar(&ast, root, GRAMMAR_TYPE_TERM);

  NodeID expr = ast_node_get_child(&ast, root, 0);
  NodeID term = ast_node_get_child(&ast, root, 1);

  cr_assert_eq(ast_node_get_child_count(&ast, expr), 0,
               "Empty grammar node should have 0 children");
  cr_assert_eq(ast_node_get_child_count(&ast, term), 0,
               "Empty grammar node should have 0 children");

  ast_destroy(&ast);
}

Test(parser, ast_node_access_all_grammar_types) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  // Test all grammar types
  GRAMMAR_TYPE types[] = {GRAMMAR_TYPE_STATEMENT,  GRAMMAR_TYPE_COMPARAISON,
                          GRAMMAR_TYPE_EXPRESSION, GRAMMAR_TYPE_TERM,
                          GRAMMAR_TYPE_UNARY,      GRAMMAR_TYPE_PRIMARY};

  size_t num_types = sizeof(types) / sizeof(types[0]);

  for (size_t i = 0; i < num_types && i < AST_MAX_CHILDREN; i++) {
    ast_node_add_child_grammar(&ast, root, types[i]);
    NodeID child = ast_node_get_child(&ast, root, (short)i);
    GRAMMAR_TYPE retrieved_type = ast_node_get_grammar(&ast, child);
    cr_assert_eq(retrieved_type, types[i], "Grammar type %zu should match", i);
  }

  ast_destroy(&ast);
}

Test(parser, ast_node_access_all_token_types) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  // Test various token types
  struct {
    enum TOKEN type;
    const char *text;
  } test_tokens[] = {{TOKEN_NUMBER, "123"},
                     {TOKEN_IDENT, "variable"},
                     {TOKEN_STRING, "hello"},
                     {TOKEN_PLUS, NULL},
                     {TOKEN_IF, NULL}};

  size_t num_tokens = sizeof(test_tokens) / sizeof(test_tokens[0]);

  for (size_t i = 0; i < num_tokens && i < AST_MAX_CHILDREN; i++) {
    Token token = create_test_token(test_tokens[i].type, test_tokens[i].text);
    ast_node_add_child_token(&ast, root, token);

    NodeID child = ast_node_get_child(&ast, root, (short)i);
    const Token *retrieved_token = ast_node_get_token(&ast, child);

    cr_assert_eq(retrieved_token->type, test_tokens[i].type,
                 "Token type %zu should match", i);
    if (test_tokens[i].text) {
      cr_assert_str_eq(retrieved_token->text, test_tokens[i].text,
                       "Token text %zu should match", i);
    } else {
      cr_assert_null(retrieved_token->text, "Token text %zu should be NULL", i);
    }

    destroy_test_token(&token);
  }

  ast_destroy(&ast);
}

// =========================
// MEMORY AND CAPACITY TESTS
// =========================

Test(parser, ast_handles_node_array_growth) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  size_t initial_capacity = ast.node_array_capacity;

  // Create a deep tree to exceed initial capacity and trigger reallocation
  // We'll create a chain: root -> grammar -> grammar -> ... -> token
  // This should easily exceed the initial capacity
  NodeID current_node = root;

  // Create enough nodes to exceed initial capacity (512)
  // Each iteration adds 1 grammar node, so we need more than 512 iterations
  int nodes_to_create =
      (int)initial_capacity + 100; // Ensure we exceed capacity

  for (int i = 0; i < nodes_to_create; i++) {
    ast_node_add_child_grammar(&ast, current_node, GRAMMAR_TYPE_EXPRESSION);
    current_node = ast_node_get_child(&ast, current_node, 0);
  }

  // Array should have grown beyond initial capacity
  cr_assert_gt(ast.node_array_capacity, initial_capacity,
               "Node array should have grown beyond initial capacity");
  cr_assert_gt(ast.node_array_size, initial_capacity,
               "Should have more nodes than initial capacity");

  // Verify we can still access the root and traverse the tree
  cr_assert(ast_node_is_grammar(&ast, root),
            "Root should still be a grammar node");
  cr_assert_eq(ast_node_get_child_count(&ast, root), 1,
               "Root should have 1 child");

  // Add a final token to the deepest node
  Token final_token = create_test_token(TOKEN_NUMBER, "deep");
  ast_node_add_child_token(&ast, current_node, final_token);

  // Verify we can access the deep token
  NodeID deep_token_node = ast_node_get_child(&ast, current_node, 0);
  const Token *retrieved_token = ast_node_get_token(&ast, deep_token_node);
  cr_assert_str_eq(retrieved_token->text, "deep",
                   "Should be able to access deeply nested token");

  ast_destroy(&ast);
  destroy_test_token(&final_token);
}

Test(parser, ast_maintains_integrity_after_reallocation) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  // Add initial token that we want to verify remains intact after reallocations
  Token first_token = create_test_token(TOKEN_IDENT, "first");
  ast_node_add_child_token(&ast, root, first_token);

  // Remember the first child's ID before reallocations
  NodeID first_child = ast_node_get_child(&ast, root, 0);

  // Add a second child to root - a grammar node that we'll use to build a deep
  // tree
  ast_node_add_child_grammar(&ast, root, GRAMMAR_TYPE_EXPRESSION);
  NodeID expr_node = ast_node_get_child(&ast, root, 1);

  // Build a deep tree from the expression node to force multiple reallocations
  NodeID current_node = expr_node;
  size_t initial_capacity = ast.node_array_capacity;

  // Create enough nodes to trigger reallocation (respecting NodeID limit)
  const size_t limit = ast.node_array_capacity * 3;
  for (size_t i = 0; i < limit; i++) {
    ast_node_add_child_grammar(&ast, current_node, GRAMMAR_TYPE_TERM);
    current_node = ast_node_get_child(&ast, current_node, 0);
  }

  // Verify that reallocation occurred
  cr_assert_gt(ast.node_array_capacity, initial_capacity,
               "Node array should have been reallocated");

  // Verify first token is still accessible and correct after reallocations
  const Token *retrieved_first = ast_node_get_token(&ast, first_child);
  cr_assert_str_eq(retrieved_first->text, "first",
                   "First token should remain correct after reallocations");

  // Verify the tree structure is still intact
  cr_assert_eq(ast_node_get_child_count(&ast, root), 2,
               "Root should still have 2 children");
  cr_assert(ast_node_is_token(&ast, first_child),
            "First child should still be a token");
  NodeID second_child = ast_node_get_child(&ast, root, 1);
  cr_assert(ast_node_is_grammar(&ast, second_child),
            "Second child should still be a grammar node");

  ast_destroy(&ast);
  destroy_test_token(&first_token);
}

// =========================
// COMPREHENSIVE INTEGRATION TESTS
// =========================

Test(parser, ast_builds_realistic_expression_tree) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  // Build: PROGRAM -> STATEMENT -> LET x = 5 + 3
  ast_node_add_child_grammar(&ast, root, GRAMMAR_TYPE_STATEMENT);
  NodeID stmt = ast_node_get_child(&ast, root, 0);

  Token let_token = create_test_token(TOKEN_LET, NULL);
  Token x_token = create_test_token(TOKEN_IDENT, "x");
  Token eq_token = create_test_token(TOKEN_EQ, NULL);

  ast_node_add_child_token(&ast, stmt, let_token);
  ast_node_add_child_token(&ast, stmt, x_token);
  ast_node_add_child_token(&ast, stmt, eq_token);

  // Add expression: 5 + 3
  ast_node_add_child_grammar(&ast, stmt, GRAMMAR_TYPE_EXPRESSION);
  NodeID expr = ast_node_get_child(&ast, stmt, 3);

  Token five_token = create_test_token(TOKEN_NUMBER, "5");
  Token plus_token = create_test_token(TOKEN_PLUS, NULL);
  Token three_token = create_test_token(TOKEN_NUMBER, "3");

  ast_node_add_child_token(&ast, expr, five_token);
  ast_node_add_child_token(&ast, expr, plus_token);
  ast_node_add_child_token(&ast, expr, three_token);

  // Verify the complete structure
  cr_assert_eq(ast_node_get_child_count(&ast, root), 1,
               "Program should have 1 statement");
  cr_assert_eq(ast_node_get_child_count(&ast, stmt), 4,
               "Statement should have 4 parts");
  cr_assert_eq(ast_node_get_child_count(&ast, expr), 3,
               "Expression should have 3 tokens");

  // Verify token contents
  const Token *let_t =
      ast_node_get_token(&ast, ast_node_get_child(&ast, stmt, 0));
  const Token *x_t =
      ast_node_get_token(&ast, ast_node_get_child(&ast, stmt, 1));
  const Token *eq_t =
      ast_node_get_token(&ast, ast_node_get_child(&ast, stmt, 2));

  cr_assert_eq(let_t->type, TOKEN_LET, "Should be LET token");
  cr_assert_str_eq(x_t->text, "x", "Should be 'x' identifier");
  cr_assert_eq(eq_t->type, TOKEN_EQ, "Should be EQ token");

  ast_destroy(&ast);
  destroy_test_token(&let_token);
  destroy_test_token(&x_token);
  destroy_test_token(&eq_token);
  destroy_test_token(&five_token);
  destroy_test_token(&plus_token);
  destroy_test_token(&three_token);
}

Test(parser, ast_handles_deeply_nested_structure) {
  AST ast = ast_init();
  NodeID root = create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);

  // Create deep nesting: PROGRAM -> STATEMENT -> EXPRESSION -> TERM -> UNARY ->
  // PRIMARY
  GRAMMAR_TYPE types[] = {GRAMMAR_TYPE_STATEMENT, GRAMMAR_TYPE_EXPRESSION,
                          GRAMMAR_TYPE_TERM, GRAMMAR_TYPE_UNARY,
                          GRAMMAR_TYPE_PRIMARY};

  NodeID current = root;
  for (size_t i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
    ast_node_add_child_grammar(&ast, current, types[i]);
    current = ast_node_get_child(&ast, current, 0);

    // Verify the grammar type
    GRAMMAR_TYPE actual_type = ast_node_get_grammar(&ast, current);
    cr_assert_eq(actual_type, types[i],
                 "Grammar type at depth %zu should match", i);
  }

  // Add final token at the deepest level
  Token final_token = create_test_token(TOKEN_NUMBER, "42");
  ast_node_add_child_token(&ast, current, final_token);

  // Verify we can access the deeply nested token
  NodeID deepest = ast_node_get_child(&ast, current, 0);
  const Token *retrieved = ast_node_get_token(&ast, deepest);
  cr_assert_str_eq(retrieved->text, "42",
                   "Deeply nested token should be accessible");

  ast_destroy(&ast);
  destroy_test_token(&final_token);
}

// Note: Tests for error conditions that cause assertions (like invalid node
// IDs, adding children to token nodes, etc.) would require more sophisticated
// testing with signal handling or fork/exec, which is beyond the scope of basic
// unit tests with Criterion. The API documentation clearly states these are
// assertion failures.