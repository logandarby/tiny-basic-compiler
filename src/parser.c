#include "parser.h"
#include "core.h"
#include "dz_debug.h"

// ====================
// CONSTANTS & DEFINITIONS
// ====================

#define INIT_NODE_ARRAY_SIZE 512
const size_t AST_RESIZE_FACTOR = 2;

typedef enum AST_NODE_TYPE {
  AST_NODE_TYPE_TOKEN,
  AST_NODE_TYPE_GRAMMAR,
} AST_NODE_TYPE;

struct ASTNode {
  AST_NODE_TYPE node_type;
  union {
    Token token; // For Leaf Nodes, which are always tokens from the lexer
    GrammarNode grammar; // For intermediary Grammer tokens
  } node;
};

// ====================
// TESTING UTIL IMPLEMENTATION
// ====================

static ASTNode *_get_node(AST *ast, NodeID node_id) {
  DZ_ASSERT(ast, "AST pointer is null");
  DZ_ASSERT(node_id < ast->node_array_size, "Node ID out of bounds");
  return &ast->node_array[node_id];
}

bool ast_node_is_token(AST *ast, NodeID node_id) {
  DZ_ASSERT(node_id < ast->node_array_size, "Invalid node ID");
  return ast->node_array[node_id].node_type == AST_NODE_TYPE_TOKEN;
}

bool ast_node_is_grammar(AST *ast, NodeID node_id) {
  ASTNode *node = _get_node(ast, node_id);
  return node->node_type == AST_NODE_TYPE_GRAMMAR;
}

NodeID ast_node_get_child(AST *ast, NodeID parent_id, short child_number) {
  DZ_ASSERT(ast_node_is_grammar(ast, parent_id),
            "Token nodes do not have children.");
  ASTNode *parent_node = _get_node(ast, parent_id);
  const GrammarNode grammar_node = parent_node->node.grammar;
  DZ_ASSERT(child_number < grammar_node.child_count,
            "Trying to access a child that cannot exist");
  const NodeID child_id = grammar_node.children[child_number];
  DZ_ASSERT(child_id < ast->node_array_size,
            "Child node does not exist in the ast");
  return child_id;
}

short ast_node_get_child_count(AST *ast, NodeID node_id) {
  if (ast_node_is_grammar(ast, node_id)) {
    ASTNode *node = _get_node(ast, node_id);
    return node->node.grammar.child_count;
  }
  return 0;
}

// Mutable version of ast_node_get_token
Token *_ast_node_get_token_mut(AST *ast, NodeID node_id) {
  DZ_ASSERT(ast_node_is_token(ast, node_id));
  ASTNode *node = _get_node(ast, node_id);
  return &(node->node.token);
}

const Token *ast_node_get_token(AST *ast, NodeID node_id) {
  return _ast_node_get_token_mut(ast, node_id);
}

// Mutable version of ast_node_get_grammar
GrammarNode *_ast_node_get_grammar_mut(AST *ast, NodeID node_id) {
  DZ_ASSERT(ast_node_is_grammar(ast, node_id));
  ASTNode *node = _get_node(ast, node_id);
  return &node->node.grammar;
}

GRAMMAR_TYPE ast_node_get_grammar(AST *ast, NodeID node_id) {
  return _ast_node_get_grammar_mut(ast, node_id)->grammar;
}

void _maybe_resize_node_array(AST *ast) {
  DZ_ASSERT(ast->node_array);
  if (ast->node_array_capacity == ast->node_array_size) {
    ast->node_array_capacity *= AST_RESIZE_FACTOR;
    ast->node_array =
        xrealloc(ast->node_array, ast->node_array_capacity * sizeof(ASTNode));
  }
}

NodeID ast_create_root_node(AST *ast, GRAMMAR_TYPE grammar_type) {
  DZ_ASSERT(ast, "AST pointer is null");
  DZ_ASSERT(ast->node_array_size == 0,
            "AST already has nodes, cannot create root");
  _maybe_resize_node_array(ast);
  GrammarNode grammar = {
      .grammar = grammar_type, .child_count = 0,
      // children array automatically zero-initialized
  };
  ASTNode root_node = {.node_type = AST_NODE_TYPE_GRAMMAR,
                       .node = {.grammar = grammar}};
  ast->node_array[0] = root_node;
  ast->node_array_size = 1;
  ast->_head = 0;
  return 0;
}

bool _node_exists(AST *ast, NodeID node_id) {
  return node_id < ast->node_array_size;
}

NodeID ast_node_add_child_token(AST *ast, NodeID parent_id, Token token) {
  GrammarNode *parent_node = _ast_node_get_grammar_mut(ast, parent_id);
  DZ_ASSERT(parent_node->child_count < AST_MAX_CHILDREN);
  _maybe_resize_node_array(ast);
  // Get grammar node pointer AFTER potential reallocation
  parent_node = _ast_node_get_grammar_mut(ast, parent_id);
  ASTNode new_token_node = {AST_NODE_TYPE_TOKEN, {.token = token}};
  NodeID new_node_id = (NodeID)ast->node_array_size;
  ast->node_array[ast->node_array_size] = new_token_node;
  parent_node->children[parent_node->child_count] = new_node_id;
  parent_node->child_count++;
  ast->node_array_size++;
  return new_node_id;
}

NodeID ast_node_add_child_grammar(AST *ast, NodeID parent_id,
                                  GRAMMAR_TYPE grammar_type) {
  GrammarNode *parent_node = _ast_node_get_grammar_mut(ast, parent_id);
  DZ_ASSERT(parent_node->child_count < AST_MAX_CHILDREN);
  _maybe_resize_node_array(ast);
  // Get grammar node pointer AFTER potential reallocation
  parent_node = _ast_node_get_grammar_mut(ast, parent_id);
  GrammarNode grammar = {
      .grammar = grammar_type, .child_count = 0,
      // children array automatically zero-initialized
  };
  ASTNode new_grammar_node = {.node_type = AST_NODE_TYPE_GRAMMAR,
                              .node = {.grammar = grammar}};
  NodeID new_node_id = (NodeID)ast->node_array_size;
  ast->node_array[ast->node_array_size] = new_grammar_node;
  parent_node->children[parent_node->child_count] = new_node_id;
  parent_node->child_count++;
  ast->node_array_size++;
  return new_node_id;
}

const char *grammar_type_to_string(GRAMMAR_TYPE type) {
  switch (type) {
  case GRAMMAR_TYPE_PROGRAM:
    return "PROGRAM";
  case GRAMMAR_TYPE_STATEMENT:
    return "STATEMENT";
  case GRAMMAR_TYPE_COMPARAISON:
    return "COMPARAISON";
  case GRAMMAR_TYPE_EXPRESSION:
    return "EXPRESSION";
  case GRAMMAR_TYPE_TERM:
    return "TERM";
  case GRAMMAR_TYPE_UNARY:
    return "UNARY";
  case GRAMMAR_TYPE_PRIMARY:
    return "PRIMARY";
  default:
    return "UNKNOWN";
  }
}
// ====================
// PUBLIC API IMPLEMENTATION
// ====================

AST ast_init(void) {
  AST ast = {
      ._head = 0,
      .node_array = xmalloc(INIT_NODE_ARRAY_SIZE * sizeof(ASTNode)),
      .node_array_capacity = INIT_NODE_ARRAY_SIZE,
      .node_array_size = 0,
  };
  return ast;
}

AST ast_parse(const TokenArray ta) {
  // Create MOCK AST representing the following program:
  // LET x = 10 + 20
  // PRINT x

  UNUSED(ta);

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

  return ast;
}

NodeID ast_head(AST ast) {
  DZ_ASSERT(ast.node_array_size > 0,
            "Tried to access AST head, when none exists");
  return ast._head;
}

void ast_destroy(AST *ast) {
  if (ast->node_array) {
    free(ast->node_array);
  }
}

bool ast_is_empty(AST *ast) { return ast->node_array_size == 0; }

// ====================
// TRAVERSAL UTILITIES
// ====================

// Helper function for ast_traverse that properly tracks context
static bool
_ast_traverse_with_context(AST *ast, NodeID current_node,
                           AstTraversalGenericContext generic_context,
                           AstTraversalVisitor *visitor, void *context) {

  // Visit the current node
  AST_TRAVERSAL_ACTION action = AST_TRAVERSAL_CONTINUE;

  if (ast_node_is_token(ast, current_node)) {
    action = visitor->visit_token(ast_node_get_token(ast, current_node),
                                  generic_context, context);
  } else {
    action = visitor->visit_grammar_enter(
        _ast_node_get_grammar_mut(ast, current_node), current_node,
        generic_context, context);
  }

  switch (action) {
  case AST_TRAVERSAL_CONTINUE:
    // Visit the children
    {
      short child_count = ast_node_get_child_count(ast, current_node);
      for (short i = 0; i < child_count; i++) {
        NodeID child_id = ast_node_get_child(ast, current_node, i);
        AstTraversalGenericContext child_context = {
            .parent_id = current_node,
            .sibling_index = i,
            .total_siblings = child_count,
        };
        const bool should_continue = _ast_traverse_with_context(
            ast, child_id, child_context, visitor, context);
        if (!should_continue) {
          return false;
        }
      }
    }
    break;
  case AST_TRAVERSAL_SKIP_CHILDREN:
    // Skip the children
    break;
  case AST_TRAVERSAL_STOP:
    // Stop the traversal
    return false;
  }

  // Exit grammar node
  if (ast_node_is_grammar(ast, current_node)) {
    action = visitor->visit_grammar_exit(
        _ast_node_get_grammar_mut(ast, current_node), current_node,
        generic_context, context);
  }

  // Return the action
  switch (action) {
  case AST_TRAVERSAL_CONTINUE:
  case AST_TRAVERSAL_SKIP_CHILDREN:
    return true;
  case AST_TRAVERSAL_STOP:
    return false;
  }
  DZ_ASSERT(false, "Unreachable");
  return true;
}

bool ast_traverse(AST *ast, NodeID start, AstTraversalVisitor *visitor,
                  void *context) {
  DZ_ASSERT(ast, "AST pointer is null");
  DZ_ASSERT(visitor, "Visitor pointer is null");
  DZ_ASSERT(context, "Context pointer is null");

  // For the root node, parent_id doesn't make sense, so we use start itself
  // and sibling_index = 0, total_siblings = 1 (the root is the only "sibling")
  AstTraversalGenericContext root_context = {
      .parent_id = start,
      .sibling_index = 0,
      .total_siblings = 1,
  };
  return _ast_traverse_with_context(ast, start, root_context, visitor, context);
}
