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
    if (shgeti(ctx->table->identifier_table, ident_token->text) == -1) {
      er_add_error(ERROR_SEMANTIC, ast_filename(ast), ident_filepos.line,
                   ident_filepos.col,
                   "The identifier %s has not been declared in the codebase",
                   ident_token->text);
      return AST_TRAVERSAL_CONTINUE;
    }
    // Error if label exists, and is wrong type
    IdentifierInfo ident_info =
        shget(ctx->table->identifier_table, ident_token->text);
    if (ident_info.type != IDENTIFIER_LABEL) {
      er_add_error(ERROR_SEMANTIC, ast_filename(ast), ident_filepos.line,
                   ident_filepos.col,
                   "The goto statement \"GOTO %s\" is pointing to a variable "
                   "instead of a label.",
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
    if (shgeti(ctx->table->identifier_table, ident_token->text) != -1) {
      IdentifierInfo info =
          shget(ctx->table->identifier_table, ident_token->text);
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
  // use,and is of the correct type
  // TODO: THIS IS WRONG and we should probably have dedicated label ident token
  // if (token->type != TOKEN_GOTO && token->type != TOKEN_LABEL && token->type
  // != TOKEN_LET) {
  //   printf("BUH\n");
  //   // the identifier is NOT a label if it does NOT come after a label or
  //   goto statement
  //   // This also checks that the variable is not a declaration using LET
  //
  //   // Then check if next one is identifier, it must then be a variable
  //   NodeID var_sibling = ast_get_next_sibling(ast, node);
  //   printf("BUH2 %d %s\n", var_sibling, ast_node_is_token(ast, var_sibling) ?
  //   "true" : "false"); if (var_sibling == NO_NODE || !ast_node_is_token(ast,
  //   var_sibling)) return AST_TRAVERSAL_CONTINUE; printf("BUH3\n"); const
  //   Token *var_token = ast_node_get_token(ast, var_sibling); if
  //   (var_token->type != TOKEN_IDENT) return AST_TRAVERSAL_CONTINUE;
  //   printf("BUH4\n");
  //   const FileLocation filepos = var_token->file_pos;
  //   printf("IS IDENT");
  //
  //   // Now, check that variable use has a corresponding declaration
  //   if (shgeti(ctx->table->identifier_table, var_token->text) == -1) {
  //     er_add_error(ERROR_SEMANTIC, ast_filename(ast), filepos.line,
  //     filepos.col, "Variable %s does not have a corresponding declartion.
  //     Please add \"LET %s = <value>\" before this line.", var_token->text,
  //     var_token->text); return AST_TRAVERSAL_CONTINUE;
  //   }
  //
  //   return AST_TRAVERSAL_CONTINUE;
  // }
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
