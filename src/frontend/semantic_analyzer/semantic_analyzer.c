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
