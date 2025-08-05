#pragma once

// ====================
// AST Traversal Visitor
//
// The AST Traversal Utils are a convenient way to traverse the AST and perform
// actions on the nodes.
//
// see ast_traverse for more details.
// ====================

#include "ast.h"

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
  (const Token *token, NodeID node_id,
   AstTraversalGenericContext generic_context, void *context);
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
