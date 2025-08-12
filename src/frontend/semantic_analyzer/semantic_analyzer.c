#include "semantic_analyzer.h"
#include "ast.h"
#include "ast_visitor.h"
#include "compiler_compatibility.h"
#include "error_reporter.h"
#include "name_table.h"
#include "token.h"
#include <stb_ds.h>

typedef struct {
  NameTable *table;
  bool success;
} Context;

AST_TRAVERSAL_ACTION _visit_token(const Token *token, NodeID node,
                                  AstTraversalGenericContext gen_ctx,
                                  void *ctx_void) {
  /*
   * Checks and reports errors for:
   *  - Inorrect identifier types (GOTO x should have x be a label, not a
   * variable)
   *  - Variable use before definition
   *  - Unknown labels
   *  - Duplicate labels
   */
  Context *ctx = (Context *)ctx_void;
  AST *ast = gen_ctx.ast;
  // for GOTO labels, check that label exists, and is of the correct type
  if (token->type == TOKEN_GOTO) {
    NodeID label_sibling = ast_get_next_sibling(ast, node);
    if (label_sibling == NO_NODE || !ast_node_is_token(ast, label_sibling))
      return AST_TRAVERSAL_STOP;
    const Token *ident_token = ast_node_get_token(ast, label_sibling);
    if (ident_token->type != TOKEN_IDENT)
      return AST_TRAVERSAL_STOP;
    FileLocation ident_filepos = ident_token->file_pos;
    // Error if label doesn't exist
    if (shgeti(ctx->table->label_table, ident_token->text) == -1) {
      er_add_error(ERROR_SEMANTIC, ast_filename(ast), ident_filepos.line,
                   ident_filepos.col,
                   "The label %s does not exist in the codebase",
                   ident_token->text);
      return AST_TRAVERSAL_CONTINUE;
    }
    return AST_TRAVERSAL_CONTINUE;
  }
  // For LABEL statements, ensure that there are no duplicates
  if (token->type == TOKEN_LABEL) {
    NodeID label_sibling = ast_get_next_sibling(ast, node);
    if (label_sibling == NO_NODE || !ast_node_is_token(ast, label_sibling))
      return AST_TRAVERSAL_STOP;
    const Token *ident_token = ast_node_get_token(ast, label_sibling);
    if (ident_token->type != TOKEN_IDENT)
      return AST_TRAVERSAL_STOP;
    FileLocation filepos = ident_token->file_pos;
    if (shgeti(ctx->table->label_table, ident_token->text) != -1) {
      IdentifierInfo info = shget(ctx->table->label_table, ident_token->text);
      // If the file positions are equal, then they refer to the same identifier
      if (memcmp(&info.file_pos, &filepos, sizeof(FileLocation)) == 0)
        return AST_TRAVERSAL_CONTINUE;
      er_add_error(
          ERROR_SEMANTIC, ast_filename(ast), filepos.line, filepos.col,
          "Duplicate label %s has already been defined at filepos %" PRIu32
          ":%" PRIu32,
          ident_token->text, info.file_pos.line, info.file_pos.col);
      return AST_TRAVERSAL_CONTINUE;
    }
    return AST_TRAVERSAL_CONTINUE;
  }
  // For variables, make sure that the variable declaration exists BEFORE
  // use
  if (token->type == TOKEN_IDENT) {
    // Make sure the identifier is not a label
    if (shgeti(ctx->table->label_table, token->text) != -1) {
      return AST_TRAVERSAL_CONTINUE;
    }
    // Also make sure the identifier is not a label
    if (gen_ctx.parent_id != NO_NODE &&
        ast_node_is_grammar(ast, gen_ctx.parent_id) &&
        ast_node_get_grammar(ast, gen_ctx.parent_id) ==
            GRAMMAR_TYPE_STATEMENT) {
      NodeID first_child = ast_get_first_child(ast, gen_ctx.parent_id);
      if (ast_node_is_token(ast, first_child)) {
        const Token *first_child_token = ast_node_get_token(ast, first_child);
        if (first_child_token->type == TOKEN_LABEL ||
            first_child_token->type == TOKEN_GOTO) {
          return AST_TRAVERSAL_CONTINUE;
        }
      }
    }
    // Check if exists
    if (shgeti(ctx->table->variable_table, token->text) == -1) {
      er_add_error(ERROR_SEMANTIC, ast_filename(ast), token->file_pos.line,
                   token->file_pos.col, "Variable %s has not been defined yet!",
                   token->text);
      return AST_TRAVERSAL_CONTINUE;
    }
    // If it exists, make sure its declared before
    FileLocation decl_filepos =
        shget(ctx->table->variable_table, token->text).file_pos;
    FileLocation current_filepos = token->file_pos;
    // TODO: Remove
    printf("Current filepos: %d:%d\t Decl filepos: %d:%d\n",
           current_filepos.line, current_filepos.col, decl_filepos.line,
           decl_filepos.col);
    if (current_filepos.line < decl_filepos.line ||
        (current_filepos.line == decl_filepos.line &&
         current_filepos.col < decl_filepos.col)) {
      er_add_error(
          ERROR_SEMANTIC, ast_filename(ast), current_filepos.line,
          current_filepos.col,
          "Variable %s used before declaration. Variable is used in %s:%" PRIu32
          ":%" PRIu32 ", but declared in %s:%" PRIu32 ":%" PRIu32,
          token->text, ast_filename(ast), current_filepos.line,
          current_filepos.col, ast_filename(ast), decl_filepos.line,
          decl_filepos.col);
      return AST_TRAVERSAL_CONTINUE;
    }
    // TODO: Special case
    // Special case: prevent user from using a variable in its own declaration
    // To do this, we see if the parent is a statement, and if the first
    // variable is a LET keyword. If so, then we see if the first sibling of the
    // let token is NOT the same as the token being analyzed. If so, then we
    // know a variable is being used in its own declaration
    // TODO: This doesn't work, we need to traverse up the tree until we find a
    // statement node
    // TODO: Add to ast api to find nearest grammar ancestor
    if (gen_ctx.parent_id == NO_NODE ||
        !ast_node_is_grammar(ast, gen_ctx.parent_id) ||
        ast_node_get_grammar(ast, gen_ctx.parent_id) !=
            GRAMMAR_TYPE_STATEMENT) {
      return AST_TRAVERSAL_CONTINUE;
    }
    printf("SPECIAL CASE\n");
    NodeID first_child = ast_get_first_child(ast, gen_ctx.parent_id);
    if (!ast_node_is_token(ast, first_child) ||
        ast_node_get_token(ast, first_child)->type != TOKEN_LET)
      return AST_TRAVERSAL_CONTINUE;
    NodeID decl_ident = ast_get_next_sibling(ast, first_child);
    if (!ast_node_is_token(ast, decl_ident))
      return AST_TRAVERSAL_CONTINUE;
    const Token *decl_ident_token = ast_node_get_token(ast, decl_ident);
    if (decl_ident_token->type != TOKEN_IDENT)
      return AST_TRAVERSAL_CONTINUE;
    if (decl_ident_token->file_pos.line == current_filepos.line &&
        decl_ident_token->file_pos.col == current_filepos.col)
      return AST_TRAVERSAL_CONTINUE;
    er_add_error(ERROR_SEMANTIC, ast_filename(ast), current_filepos.line,
                 current_filepos.col,
                 "Variable %s is referenced in its own declaration.",
                 token->text);
  }
  return AST_TRAVERSAL_CONTINUE;
}

bool semantic_analyzer_check(AST *ast, NameTable *table) {
  AstTraversalVisitor visitor = {
      .visit_grammar_enter = NULL,
      .visit_grammar_exit = NULL,
      .visit_token = _visit_token,
  };
  Context ctx = {.success = true, .table = table};
  NodeID head = ast_head(ast);
  if (head == NO_NODE)
    return true;
  ast_traverse(ast, head, &visitor, &ctx);
  return ctx.success;
}
