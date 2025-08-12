#include "ast_visitor.h"
#include "arg_parse.h"
#include "ast.h"

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
  if (current_node == NO_NODE)
    return AST_TRAVERSAL_CONTINUE;
  if (ast_node_is_token(ast, current_node) && visitor->visit_token != NULL) {
    action = visitor->visit_token(ast_node_get_token(ast, current_node),
                                  current_node, generic_context, context);
  } else if (ast_node_is_grammar(ast, current_node) &&
             visitor->visit_grammar_enter != NULL) {
    action = visitor->visit_grammar_enter(
        ast_node_get_grammar_mut(ast, current_node), current_node,
        generic_context, context);
  }

  switch (action) {
  case AST_TRAVERSAL_CONTINUE:
    // Visit the children
    {
      NodeID current_child = ast_get_first_child(ast, current_node);
      while (current_child != NO_NODE) {
        AstTraversalGenericContext child_context = {
            .ast = ast, .parent_id = current_node, .node_id = current_child};
        const bool should_continue = _ast_traverse_with_context(
            ast, current_child, child_context, visitor, context);
        if (!should_continue) {
          return false;
        }
        current_child = ast_get_next_sibling(ast, current_child);
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
  if (ast_node_is_grammar(ast, current_node) &&
      visitor->visit_grammar_exit != NULL) {
    action =
        visitor->visit_grammar_exit(ast_node_get_grammar_mut(ast, current_node),
                                    current_node, generic_context, context);
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

  AstTraversalGenericContext root_context = {
      .parent_id = NO_NODE,
      .node_id = start,
      .ast = ast,
  };
  return _ast_traverse_with_context(ast, start, root_context, visitor, context);
}
