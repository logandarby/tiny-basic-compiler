#pragma once

// ------------------------------------
// AST & PARSER
//
// The AST is a tree structure that represents the program.
// The parser is used to parse the program into an AST.
// ------------------------------------

#include "../core/core.h"
#include "../frontend/lexer/token.h"

// ====================
// AST & PARSER DEFINITIONS
// ====================

typedef struct ASTNode ASTNode;
// Index into out-of-band array with ASTNode struct
typedef uint32_t NodeID;
// Represents when no node is present
extern const NodeID NO_NODE;
;

// ========================
// AST DEFINITIONS
// ========================

typedef enum GRAMMAR_TYPE {
  GRAMMAR_TYPE_PROGRAM,
  GRAMMAR_TYPE_STATEMENT,
  GRAMMAR_TYPE_COMPARISON,
  GRAMMAR_TYPE_EXPRESSION,
  GRAMMAR_TYPE_TERM,
  GRAMMAR_TYPE_UNARY,
  GRAMMAR_TYPE_PRIMARY,
} GRAMMAR_TYPE;

typedef struct {
  GRAMMAR_TYPE grammar;
  NodeID first_child;
  NodeID last_child;
} GrammarNode;

// An AST is a collection of ASTNodes.
// It stores the head of the AST, and a pointer to the out-of-band array of
// ASTNodes.
typedef struct {
  NodeID _head;
  // Stores the ASTNodes out of band -- dynamically reallocates
  ASTNode *node_array;
  uint32_t node_array_capacity;
  uint32_t node_array_size;
} AST;

typedef enum AST_NODE_TYPE {
  AST_NODE_TYPE_TOKEN,
  AST_NODE_TYPE_GRAMMAR,
} AST_NODE_TYPE;

// An ASTNode is a discriminated union of a Token or a GrammarNode.
// A token is a leaf node which is always a token from the lexer.
// A GrammarNode is a potentially non-leaf node which is a grammar rule from the
// grammar rules above. It is used to store the AST in a single array.
struct ASTNode {
  AST_NODE_TYPE node_type;
  union {
    Token token; // For Leaf Nodes, which are always tokens from the lexer
    GrammarNode grammar; // For intermediary Grammer tokens
  } node;
  NodeID next_sibling; // Optionally points to the next sibling in the AST,
                       // otherwise NO_NODE
};

void ast_destroy(AST *ast);
// The root of the ast
NodeID ast_head(AST *ast);
bool ast_is_empty(AST *ast);
// Creates an empty Abstract Syntax Tree, used only for testing really
AST ast_init(void);
// Creates a root node. Only can do so on an empty tree
NodeID ast_create_root_node(AST *ast, GRAMMAR_TYPE grammar_type);
// Returns if the node is a token
bool ast_node_is_token(AST *ast, NodeID node_id);
// Returns if the node is a grammar node
bool ast_node_is_grammar(AST *ast, NodeID node_id);
// These method adds a child token to a node. Note that adding children only
// works on GrammarNodes -- you should check this before calling this method
NodeID ast_node_add_child_token(AST *ast, NodeID parent_id, Token token);
// These method adds a child gramamr node to a node. Note that adding children
// only works on GrammarNodes -- you should check this before calling this
// method
NodeID ast_node_add_child_grammar(AST *ast, NodeID parent_id,
                                  GRAMMAR_TYPE grammar_type);
// If the node is a Token node, it gets the Token. Otherwise, it panics
const Token *ast_node_get_token(AST *ast, NodeID node_id);
// If the node is a grammar node, retrives the grammar type. Otherwise, it
// panics.
GRAMMAR_TYPE ast_node_get_grammar(AST *ast, NodeID node_id);
GrammarNode *ast_node_get_grammar_mut(AST *ast, NodeID node_id);
NodeID ast_get_first_child(AST *ast, NodeID node);
NodeID ast_get_next_sibling(AST *ast, NodeID node);
const char *grammar_type_to_string(GRAMMAR_TYPE type);

// ==============================
// TESTING UTIL
//
// These utils are used strictly for testing if possible considering their
// inefficiency.
// =============================

#ifdef DZ_TESTING
// Gets the child of a node. Only works on GrammarNodes-- you should check that
// the node is a grammar node before calling thing
NodeID ast_node_get_child(AST *ast, NodeID parent_id, short child_number);
// Get the # of children the node has
uint32_t ast_node_get_child_count(AST *ast, NodeID node_id);
#endif
