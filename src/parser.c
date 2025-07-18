#include "parser.h"
#include "core.h"
#include "dz_debug.h"

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

static ASTNode *_get_node(AST *ast, NodeID node_id) {
  DZ_ASSERT(ast, "AST pointer is null");
  DZ_ASSERT(node_id < ast->node_array_size, "Node ID out of bounds");
  return &ast->node_array[node_id];
}

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
  UNUSED(ta);
  DZ_THROW("Not implemented yet");
  return ast_init();
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

void ast_node_add_child_token(AST *ast, NodeID parent_id, Token token) {
  GrammarNode *parent_node = _ast_node_get_grammar_mut(ast, parent_id);
  DZ_ASSERT(parent_node->child_count < AST_MAX_CHILDREN);
  _maybe_resize_node_array(ast);
  // Get grammar node pointer AFTER potential reallocation
  parent_node = _ast_node_get_grammar_mut(ast, parent_id);
  ASTNode new_token_node = {AST_NODE_TYPE_TOKEN, {.token = token}};
  ast->node_array[ast->node_array_size] = new_token_node;
  parent_node->children[parent_node->child_count] =
      (NodeID)ast->node_array_size;
  parent_node->child_count++;
  ast->node_array_size++;
}

void ast_node_add_child_grammar(AST *ast, NodeID parent_id,
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
  ast->node_array[ast->node_array_size] = new_grammar_node;
  parent_node->children[parent_node->child_count] =
      (NodeID)ast->node_array_size;
  parent_node->child_count++;
  ast->node_array_size++;
}
