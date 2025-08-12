#include "name_table.h"
#include "../ast/ast_visitor.h"
#include "ast.h"
#include "token.h"
#include <stb_ds.h>
#include <stdlib.h>

typedef struct {
  uint32_t counter;
  NameTable *table;
} Ctx;

AST_TRAVERSAL_ACTION visit_token(const Token *token, NodeID node_id,
                                 AstTraversalGenericContext gen_ctx,
                                 void *ctx_void) {
  // If the token is a let statement, then sees if next is identifier and adds
  // it Id the token is a string -- adds literal to table
  Ctx *ctx = (Ctx *)ctx_void;
  NameTable *table = ctx->table;
  // Add string token
  if (token->type == TOKEN_STRING) {
    LiteralInfo str_info = {
        .label = ctx->counter,
        .file_pos = token->file_pos,
    };
    shput(table->literal_table, token->text, str_info);
    ctx->counter++;
    return AST_TRAVERSAL_CONTINUE;
  }
  // If the token is a label keyword, then sees if the next is an identifier and
  // adds it to the label table
  if (token->type == TOKEN_LABEL) {
    const NodeID sibling = ast_get_next_sibling(gen_ctx.ast, node_id);
    if (sibling == NO_NODE || !ast_node_is_token(gen_ctx.ast, sibling))
      return AST_TRAVERSAL_CONTINUE;
    const Token *ident_token = ast_node_get_token(gen_ctx.ast, sibling);
    if (ident_token->type != TOKEN_IDENT)
      return AST_TRAVERSAL_CONTINUE;
    // Add label
    LabelInfo new_label = {.file_pos = token->file_pos};
    shput(table->label_table, ident_token->text, new_label);
    return AST_TRAVERSAL_CONTINUE;
  }
  // Check if the token is "LET" with an identifier sibling, and add it
  if (token->type == TOKEN_LET) {
    const NodeID sibling = ast_get_next_sibling(gen_ctx.ast, node_id);
    if (sibling == NO_NODE || !ast_node_is_token(gen_ctx.ast, sibling))
      return AST_TRAVERSAL_CONTINUE;
    const Token *ident_token = ast_node_get_token(gen_ctx.ast, sibling);
    if (ident_token->type != TOKEN_IDENT)
      return AST_TRAVERSAL_CONTINUE;
    // Add identifier
    SymbolInfo new_symbol = {.file_pos = token->file_pos};
    shput(table->symbol_table, ident_token->text, new_symbol);
    ctx->counter++;
    return AST_TRAVERSAL_CONTINUE;
  }
  return AST_TRAVERSAL_CONTINUE;
}

static AstTraversalVisitor variable_visitor = {
    .visit_grammar_enter = NULL,
    .visit_grammar_exit = NULL,
    .visit_token = visit_token,
};

NameTable *name_table_collect_from_ast(AST *ast) {
  NameTable *table = malloc(sizeof(NameTable));
  *table = (NameTable){
      .literal_table = NULL,
      .symbol_table = NULL,
  };
  Ctx ctx = (Ctx){
      .counter = 0,
      .table = table,
  };
  ast_traverse(ast, ast_head(ast), &variable_visitor, &ctx);
  return table;
}

void name_table_destroy(NameTable *var_table) {
  if (!var_table)
    return;
  shfree(var_table->symbol_table);
  shfree(var_table->literal_table);
  shfree(var_table->label_table);
  free(var_table);
}
