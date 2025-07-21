#pragma once

// ---------------------------------------------
// PARSER
//
// Given a list of Tokens from the Lexer, the Parser turns
// the list of tokens into an AST based off the Tiny BASIC grammar
// ---------------------------------------------

#include "../lexer.h"

/*
GRAMMAR DEFINITION

program ::= {statement}*
statement ::= "PRINT" (expression | string) nl
    | "IF" comparison "THEN" nl {statement}* "ENDIF" nl
    | "WHILE" comparison "REPEAT" nl {statement}* "ENDWHILE" nl
    | "LABEL" ident nl
    | "GOTO" ident nl
    | "LET" ident "=" expression nl
    | "INPUT" ident nl
comparison ::= expression (("==" | "!=" | ">" | ">=" | "<" | "<=") expression)+
expression ::= term {( "-" | "+" ) term}
term ::= unary {( "/" | "*" ) unary}
unary ::= ["+" | "-"] primary
primary ::= number | ident
nl ::= '\n'+
*/

// ====================
// AST & PARSER DEFINITIONS
// ====================

typedef struct ASTNode ASTNode;
// Index into out-of-band array with ASTNode struct
typedef size_t NodeID;
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
  size_t node_array_capacity;
  size_t node_array_size;
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

// Initializes an AST and parses the TokenArray according to the
// grammar rules above.
// The AST must be destroyed with the ast_destroy function
AST ast_parse(const TokenArray ta);
void ast_destroy(AST *ast);
// The root of the ast
NodeID ast_head(AST ast);
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
NodeID ast_get_first_child(AST *ast, NodeID node);
NodeID ast_get_next_sibling(AST *ast, NodeID node);

// ====================
// AST Traversal Visitor
//
// The AST Traversal Utils are a convenient way to traverse the AST and perform
// actions on the nodes.
//
// see ast_traverse for more details.
// ====================

typedef enum {
  AST_TRAVERSAL_CONTINUE,
  AST_TRAVERSAL_STOP,
  AST_TRAVERSAL_SKIP_CHILDREN,
} AST_TRAVERSAL_ACTION;

typedef struct {
  NodeID node_id;
  NodeID parent_id;
  AST *ast;
} AstTraversalGenericContext;

// A visitor is a function that is called for each node in the AST.
// You must implement all three functions.
// The generic context is passed to the visitor, which contains information such
// as the sibling index, the parent node, and the total number of siblings.
// The context is passed to the visitor, which is a user-defined context that
// is passed to the visitor.
typedef struct {
  AST_TRAVERSAL_ACTION(*visit_token)
  (const Token *token, AstTraversalGenericContext generic_context,
   void *context);
  AST_TRAVERSAL_ACTION(*visit_grammar_enter)
  (GrammarNode *grammar, NodeID node_id,
   AstTraversalGenericContext generic_context, void *context);
  AST_TRAVERSAL_ACTION(*visit_grammar_exit)
  (GrammarNode *grammar, NodeID node_id,
   AstTraversalGenericContext generic_context, void *context);
} AstTraversalVisitor;

// Traverses the AST starting from the given node, from left to right.
// Does a depth-first traversal.
// Returns true if the traversal was successful, false otherwise.
// The visitor is called for each node in the AST.
// The context is passed to the visitor.
// The visitor can return one of the following actions:
// - AST_TRAVERSAL_CONTINUE: Continue the traversal.
// - AST_TRAVERSAL_STOP: Stop the traversal.
// - AST_TRAVERSAL_SKIP_CHILDREN: Skip the children of the current node, and go
// on to the next sibling.
bool ast_traverse(AST *ast, NodeID start, AstTraversalVisitor *visitor,
                  void *context);

// Prints the AST. An example of the traversal pattern.
void ast_print(AST *ast);

// A testing utility used to verify the structure of the AST.
// Example:
// ast_verify_structure(&ast,
//  "PROGRAM(STATEMENT(LET,IDENT(x),EQ,EXPRESSION(NUMBER(5),PLUS,NUMBER(3))))"
// );
bool ast_verify_structure(AST *ast, const char *expected_structure);

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
size_t ast_node_get_child_count(AST *ast, NodeID node_id);
#endif
