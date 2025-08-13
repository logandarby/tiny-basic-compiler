#include "name_table.h"
#include "../ast/ast_visitor.h"
#include "ast.h"
#include "token.h"
#include <stb_ds.h>
#include <stdlib.h>

typedef struct {
  uint32_t counter;
  NameTable *table;
  NodeID *statement_stack; // Keeps track of statements as nodes are visited in
                           // a stack
} Ctx;

NodeID get_statement_ancestor(Ctx *ctx) {
  if (arrlen(ctx->statement_stack) == 0)
    return NO_NODE;
  return arrlast(ctx->statement_stack);
}

static AST_TRAVERSAL_ACTION _enter_grammar(GrammarNode *grammar,
                                           const NodeID node,
                                           AstTraversalGenericContext gen_ctx,
                                           void *ctx_void) {
  UNUSED(gen_ctx);
  Ctx *ctx = (Ctx *)ctx_void;
  if (grammar->grammar == GRAMMAR_TYPE_STATEMENT) {
    arrpush(ctx->statement_stack, node);
  }
  return AST_TRAVERSAL_CONTINUE;
}

static AST_TRAVERSAL_ACTION _exit_grammar(GrammarNode *grammar,
                                          const NodeID node,
                                          AstTraversalGenericContext gen_ctx,
                                          void *ctx_void) {
  UNUSED(node);
  UNUSED(gen_ctx);
  Ctx *ctx = (Ctx *)ctx_void;
  if (grammar->grammar == GRAMMAR_TYPE_STATEMENT) {
    NodeID _ = arrpop(ctx->statement_stack);
    UNUSED(_);
  }
  return AST_TRAVERSAL_CONTINUE;
}

AST_TRAVERSAL_ACTION visit_token(const Token *token, NodeID node_id,
                                 AstTraversalGenericContext gen_ctx,
                                 void *ctx_void) {
  // If the token is a LET statement, then sees if next is identifier and adds
  // it as a variable. If it occurs after a LABEL statement, adds it as a label
  // type. If the token is a string -- adds literal to table
  Ctx *ctx = (Ctx *)ctx_void;
  NameTable *table = ctx->table;
  // ADD LITERAL
  // Add string token
  if (token->type == TOKEN_STRING) {
    // Check if it exists already
    if (shgetp_null(table->literal_table, token->text) != NULL)
      return AST_TRAVERSAL_CONTINUE;
    LiteralInfo str_info = {
        .label = ctx->counter,
        .file_pos = token->file_pos,
    };
    shput(table->literal_table, token->text, str_info);
    ctx->counter++;
    return AST_TRAVERSAL_CONTINUE;
  }
  // ADD LABEL DECLARATION
  // If the token is a label keyword, then sees if the next is an identifier and
  // adds it to the label table
  if (token->type == TOKEN_LABEL) {
    const NodeID sibling = ast_get_next_sibling(gen_ctx.ast, node_id);
    if (sibling == NO_NODE || !ast_node_is_token(gen_ctx.ast, sibling))
      return AST_TRAVERSAL_CONTINUE;
    const Token *ident_token = ast_node_get_token(gen_ctx.ast, sibling);
    if (ident_token->type != TOKEN_IDENT)
      return AST_TRAVERSAL_CONTINUE;
    // Check if it exists already
    if (shgetp_null(table->label_table, ident_token->text) != NULL)
      return AST_TRAVERSAL_CONTINUE;
    // Add label
    IdentifierInfo new_label = {.file_pos = ident_token->file_pos,
                                .parent_statement =
                                    get_statement_ancestor(ctx)};
    shput(table->label_table, ident_token->text, new_label);
    return AST_TRAVERSAL_CONTINUE;
  }
  // ADD VARIABLE DECLARATION
  // Check if the token is "LET" with an identifier sibling, and add it
  if (token->type == TOKEN_LET) {
    const NodeID sibling = ast_get_next_sibling(gen_ctx.ast, node_id);
    if (sibling == NO_NODE || !ast_node_is_token(gen_ctx.ast, sibling))
      return AST_TRAVERSAL_CONTINUE;
    const Token *ident_token = ast_node_get_token(gen_ctx.ast, sibling);
    if (ident_token->type != TOKEN_IDENT)
      return AST_TRAVERSAL_CONTINUE;
    // Check if it exists already
    if (shgetp_null(table->variable_table, ident_token->text) != NULL)
      return AST_TRAVERSAL_CONTINUE;
    // Add identifier
    IdentifierInfo new_symbol = {.file_pos = ident_token->file_pos,
                                 .parent_statement =
                                     get_statement_ancestor(ctx)};
    shput(table->variable_table, ident_token->text, new_symbol);
    ctx->counter++;
    return AST_TRAVERSAL_CONTINUE;
  }
  return AST_TRAVERSAL_CONTINUE;
}

static AstTraversalVisitor variable_visitor = {
    .visit_grammar_enter = _enter_grammar,
    .visit_grammar_exit = _exit_grammar,
    .visit_token = visit_token,
};

NameTable *name_table_collect_from_ast(AST *ast) {
  NameTable *table = malloc(sizeof(NameTable));
  *table = (NameTable){
      .literal_table = NULL,
      .variable_table = NULL,
      .label_table = NULL,
  };
  Ctx ctx = (Ctx){
      .counter = 0,
      .table = table,
      .statement_stack = NULL,
  };
  ast_traverse(ast, ast_head(ast), &variable_visitor, &ctx);
  arrfree(ctx.statement_stack);
  return table;
}

void name_table_destroy(NameTable *var_table) {
  if (!var_table)
    return;
  shfree(var_table->variable_table);
  shfree(var_table->label_table);
  shfree(var_table->literal_table);
  free(var_table);
}
