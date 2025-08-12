#include "ast.h"
#include <string.h>

// ====================
// CONSTANTS & DEFINITIONS
// ====================

#define INIT_NODE_ARRAY_SIZE 512
const uint32_t AST_RESIZE_FACTOR = 2;
const NodeID NO_NODE = ((NodeID)-1);

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
GrammarNode *ast_node_get_grammar_mut(AST *ast, NodeID node_id) {
  DZ_ASSERT(ast_node_is_grammar(ast, node_id));
  ASTNode *node = _get_node(ast, node_id);
  return &node->node.grammar;
}

GRAMMAR_TYPE ast_node_get_grammar(AST *ast, NodeID node_id) {
  return ast_node_get_grammar_mut(ast, node_id)->grammar;
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
      .grammar = grammar_type,
      .first_child = NO_NODE,
      .last_child = NO_NODE,
  };
  ASTNode root_node = {.node_type = AST_NODE_TYPE_GRAMMAR,
                       .node = {.grammar = grammar},
                       .next_sibling = NO_NODE};
  ast->node_array[0] = root_node;
  ast->node_array_size = 1;
  ast->_head = 0;
  return 0;
}

bool _node_exists(AST *ast, NodeID node_id) {
  return node_id < ast->node_array_size;
}

// Requires the new_node_id already exists in the out-of-band array
void _ast_add_node_to_linked_list(AST *ast, GrammarNode *parent_node,
                                  NodeID new_node_id) {
  // If the parent node has children, we extend the linked list
  if (parent_node->last_child != NO_NODE) {
    ASTNode *prev_last_child = &ast->node_array[parent_node->last_child];
    prev_last_child->next_sibling = new_node_id;
    parent_node->last_child = new_node_id;
  } else {
    // Else, we create the linked list
    parent_node->first_child = new_node_id;
    parent_node->last_child = new_node_id;
  }
}

NodeID ast_node_add_child_token(AST *ast, NodeID parent_id, Token token) {
  GrammarNode *parent_node = ast_node_get_grammar_mut(ast, parent_id);
  _maybe_resize_node_array(ast);
  // Get grammar node pointer AFTER potential reallocation
  parent_node = ast_node_get_grammar_mut(ast, parent_id);
  ASTNode new_token_node = {
      AST_NODE_TYPE_TOKEN,
      {.token = token},
      .next_sibling = NO_NODE // Initialized later
  };
  NodeID new_node_id = (NodeID)ast->node_array_size;
  ast->node_array[ast->node_array_size] = new_token_node;
  ast->node_array_size++;
  _ast_add_node_to_linked_list(ast, parent_node, new_node_id);
  return new_node_id;
}

NodeID ast_node_add_child_grammar(AST *ast, NodeID parent_id,
                                  GRAMMAR_TYPE grammar_type) {
  GrammarNode *parent_node = ast_node_get_grammar_mut(ast, parent_id);
  _maybe_resize_node_array(ast);
  // Get grammar node pointer AFTER potential reallocation
  parent_node = ast_node_get_grammar_mut(ast, parent_id);
  GrammarNode grammar = {
      .grammar = grammar_type,
      .first_child = NO_NODE,
      .last_child = NO_NODE,
  };
  ASTNode new_grammar_node = {.node_type = AST_NODE_TYPE_GRAMMAR,
                              .node = {.grammar = grammar},
                              .next_sibling = NO_NODE};
  NodeID new_node_id = (NodeID)ast->node_array_size;
  ast->node_array[ast->node_array_size] = new_grammar_node;
  ast->node_array_size++;
  _ast_add_node_to_linked_list(ast, parent_node, new_node_id);
  return new_node_id;
}

const char *grammar_type_to_string(GRAMMAR_TYPE type) {
  switch (type) {
  case GRAMMAR_TYPE_PROGRAM:
    return "PROGRAM";
  case GRAMMAR_TYPE_STATEMENT:
    return "STATEMENT";
  case GRAMMAR_TYPE_COMPARISON:
    return "COMPARISON";
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

NodeID ast_head(AST *ast) {
  DZ_ASSERT(ast);
  DZ_ASSERT(ast->node_array_size > 0,
            "Tried to access AST head, when none exists");
  return ast->_head;
}

void ast_destroy(AST *ast) {
  if (ast->node_array) {
    free(ast->node_array);
  }
  if (ast->filename) {
    free(ast->filename);
  }
}

bool ast_is_empty(AST *ast) { return ast->node_array_size == 0; }

NodeID ast_get_first_child(AST *ast, NodeID node) {
  if (node == NO_NODE || !ast_node_is_grammar(ast, node)) {
    return NO_NODE;
  }
  return ast_node_get_grammar_mut(ast, node)->first_child;
}

NodeID ast_get_next_sibling(AST *ast, NodeID node) {
  if (node == NO_NODE)
    return NO_NODE;
  return ast->node_array[node].next_sibling;
}

// ====================
// TESTING UTIL IMPLEMENTATION
// ====================

uint32_t ast_node_get_child_count(AST *ast, NodeID node_id) {
  if (ast_node_is_grammar(ast, node_id)) {
    ASTNode *node = _get_node(ast, node_id);
    NodeID current_child = node->node.grammar.first_child;
    uint32_t total_children = 0;
    while (current_child != NO_NODE) {
      total_children++;
      current_child = ast_get_next_sibling(ast, current_child);
    }
    return total_children;
  }
  return 0;
}

NodeID ast_node_get_child(AST *ast, NodeID parent_id, short child_number) {
  DZ_ASSERT(ast_node_is_grammar(ast, parent_id),
            "Token nodes do not have children.");
  ASTNode *parent_node = _get_node(ast, parent_id);
  const GrammarNode grammar_node = parent_node->node.grammar;
  NodeID current_child = grammar_node.first_child;
  for (short i = 0; i < child_number; i++) {
    DZ_ASSERT(current_child != NO_NODE,
              "Child node number does not exist as a child of the parent node");
    current_child = ast_get_next_sibling(ast, current_child);
  }
  DZ_ASSERT(current_child < ast->node_array_size,
            "Child node does not exist in the ast");
  return current_child;
}

const char *ast_filename(const AST *ast) { return ast->filename; }

void ast_set_filename(AST *ast, const char *filename) {
  if (ast->filename) {
    free(ast->filename);
  }
  ast->filename = strdup(filename);
}
