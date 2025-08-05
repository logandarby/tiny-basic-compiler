#include "symbol_table.h"
#include "../ast/ast_visitor.h"
#include "ast.h"
#include "token.h"
#include <stb_ds.h>
#include <stdlib.h>

typedef struct {
  size_t counter;
  VariableTable *table;
} Ctx;

AST_TRAVERSAL_ACTION visit_token(const Token *token, NodeID node_id,
                                 AstTraversalGenericContext gen_ctx,
                                 void *ctx_void) {
  // If the token is "LET" -- looks for sibling with identifier and adds it
  // Id the token is a string -- adds literal to table
  Ctx *ctx = (Ctx *)ctx_void;
  VariableTable *table = ctx->table;
  // Add string token
  if (token->type == TOKEN_STRING) {
    shput(table->literal_table, token->text,
          (LiteralInfo){.label = ctx->counter});
    ctx->counter++;
    return AST_TRAVERSAL_CONTINUE;
  }
  // Check if the token is "LET" with an identifier sibling, and add it
  if (token->type != TOKEN_LET)
    return AST_TRAVERSAL_CONTINUE;
  const NodeID sibling = ast_get_next_sibling(gen_ctx.ast, node_id);
  if (sibling == NO_NODE || !ast_node_is_token(gen_ctx.ast, sibling))
    return AST_TRAVERSAL_CONTINUE;
  const Token *ident_token = ast_node_get_token(gen_ctx.ast, sibling);
  if (ident_token->type != TOKEN_IDENT)
    return AST_TRAVERSAL_CONTINUE;
  // Add identifier
  SymbolInfo new_symbol = {
      .label = ctx->counter,
  };
  shput(table->symbol_table, ident_token->text, new_symbol);
  ctx->counter++;
  return AST_TRAVERSAL_CONTINUE;
}

static AstTraversalVisitor variable_visitor = {
    .visit_grammar_enter = NULL,
    .visit_grammar_exit = NULL,
    .visit_token = visit_token,
};

VariableTable *variables_collect_from_ast(AST *ast) {
  VariableTable *table = malloc(sizeof(VariableTable));
  *table = (VariableTable){
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

void variables_destroy(VariableTable *var_table) {
  if (!var_table)
    return;
  shfree(var_table->symbol_table);
  shfree(var_table->literal_table);
  free(var_table);
}
