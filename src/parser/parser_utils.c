#include "parser.h"

static void _print_indent_with_tree(size_t indent) {
  for (size_t i = 0; i < indent; i++) {
    printf("  "); // Vertical line with spacing
  }
}

static AST_TRAVERSAL_ACTION
_print_token(const Token *token, AstTraversalGenericContext generic_context,
             void *context) {
  UNUSED(generic_context);
  size_t indent = *(size_t *)context;
  _print_indent_with_tree(indent);
  if (token->text) {
    printf("TOKEN(%s): %s\n", token_type_to_string(token->type), token->text);
  } else {
    printf("TOKEN(%s)\n", token_type_to_string(token->type));
  }
  return AST_TRAVERSAL_CONTINUE;
}

static AST_TRAVERSAL_ACTION
_print_grammar_enter(GrammarNode *grammar, NodeID node_id,
                     AstTraversalGenericContext generic_context,
                     void *context) {
  UNUSED(node_id);
  UNUSED(generic_context);
  size_t *indent = (size_t *)context;
  _print_indent_with_tree(*indent);
  printf("<%s>\n", grammar_type_to_string(grammar->grammar));
  (*indent)++;
  return AST_TRAVERSAL_CONTINUE;
}

static AST_TRAVERSAL_ACTION
_print_grammar_exit(GrammarNode *grammar, NodeID node_id,
                    AstTraversalGenericContext generic_context, void *context) {
  UNUSED(node_id);
  UNUSED(grammar);
  UNUSED(generic_context);
  size_t *indent = (size_t *)context;
  (*indent)--;
  return AST_TRAVERSAL_CONTINUE;
}

void ast_print(AST *ast) {
  AstTraversalVisitor visitor = {
      .visit_token = _print_token,
      .visit_grammar_enter = _print_grammar_enter,
      .visit_grammar_exit = _print_grammar_exit,
  };
  size_t indent = 0;
  if (ast_is_empty(ast)) {
    printf("<EMPTY>\n");
  } else {
    ast_traverse(ast, ast_head(*ast), &visitor, &indent);
  }
}

typedef struct {
  char *str;
  size_t str_len;
  size_t str_capacity;
} BracketPrintContext;

// write to and automatically resize the string in the context
static void _write_to_bracket_context(BracketPrintContext *ctx,
                                      const char *str) {
  size_t str_len = strlen(str);
  if (ctx->str_len + str_len + 1 >= ctx->str_capacity) {
    ctx->str_capacity *= 2;
    ctx->str = xrealloc(ctx->str, ctx->str_capacity);
  }
  strncpy(ctx->str + ctx->str_len, str, str_len);
  ctx->str_len += str_len;
  ctx->str[ctx->str_len] = '\0';
}

static AST_TRAVERSAL_ACTION
_print_bracket_grammar_enter(GrammarNode *grammar, NodeID node_id,
                             AstTraversalGenericContext generic_context,
                             void *context) {
  UNUSED(node_id);
  UNUSED(generic_context);
  BracketPrintContext *ctx = (BracketPrintContext *)context;
  const char *grammar_str = grammar_type_to_string(grammar->grammar);
  _write_to_bracket_context(ctx, grammar_str);
  _write_to_bracket_context(ctx, "(");
  return AST_TRAVERSAL_CONTINUE;
}

static AST_TRAVERSAL_ACTION
_print_bracket_grammar_exit(GrammarNode *grammar, NodeID node_id,
                            AstTraversalGenericContext generic_context,
                            void *context) {
  UNUSED(node_id);
  UNUSED(generic_context);
  UNUSED(grammar);
  BracketPrintContext *ctx = (BracketPrintContext *)context;
  _write_to_bracket_context(ctx, ")");
  // if its not the last sibling, add a comma
  if (generic_context.sibling_index < generic_context.total_siblings - 1) {
    _write_to_bracket_context(ctx, ",");
  }
  return AST_TRAVERSAL_CONTINUE;
}

static AST_TRAVERSAL_ACTION
_print_bracket_token(const Token *token,
                     AstTraversalGenericContext generic_context,
                     void *context) {
  UNUSED(generic_context);
  BracketPrintContext *ctx = (BracketPrintContext *)context;
  const char *token_str = token_type_to_string(token->type);
  _write_to_bracket_context(ctx, token_str);
  if (token->text) {
    _write_to_bracket_context(ctx, "(");
    _write_to_bracket_context(ctx, token->text);
    _write_to_bracket_context(ctx, ")");
  }
  // if its not the last sibling, add a comma
  if (generic_context.sibling_index < generic_context.total_siblings - 1) {
    _write_to_bracket_context(ctx, ",");
  }
  return AST_TRAVERSAL_CONTINUE;
}

char *ast_bracket_print(AST *ast) {
  BracketPrintContext ctx = {
      .str = xmalloc(10), .str_len = 0, .str_capacity = 10};
  AstTraversalVisitor visitor = {
      .visit_token = _print_bracket_token,
      .visit_grammar_enter = _print_bracket_grammar_enter,
      .visit_grammar_exit = _print_bracket_grammar_exit,
  };
  if (ast_is_empty(ast)) {
    return strdup("<EMPTY>");
  }
  ast_traverse(ast, ast_head(*ast), &visitor, &ctx);
  return ctx.str;
}

typedef struct {
  const char *expected_pattern;
  const char *current_pos;
  bool matches;
  char error_msg[256];
} ASTVerificationContext;

static AST_TRAVERSAL_ACTION
_verify_grammar_enter(GrammarNode *grammar, NodeID node_id,
                      AstTraversalGenericContext generic_context,
                      void *context) {
  UNUSED(node_id);
  UNUSED(generic_context);
  ASTVerificationContext *ctx = (ASTVerificationContext *)context;
  const char *expected_grammar_str = grammar_type_to_string(grammar->grammar);
  if (strncmp(ctx->current_pos, expected_grammar_str,
              strlen(expected_grammar_str)) != 0) {
    ctx->matches = false;
    snprintf(ctx->error_msg, sizeof(ctx->error_msg),
             "Expected grammar type: %s, got: %s", expected_grammar_str,
             ctx->current_pos);
    return AST_TRAVERSAL_STOP;
  }
  ctx->current_pos += strlen(expected_grammar_str);
  if (ctx->current_pos[0] != '(') {
    ctx->matches = false;
    snprintf(ctx->error_msg, sizeof(ctx->error_msg), "Expected '(', got: %s",
             ctx->current_pos);
    return AST_TRAVERSAL_STOP;
  }
  ctx->current_pos++;
  return AST_TRAVERSAL_CONTINUE;
}

static AST_TRAVERSAL_ACTION
_verify_grammar_exit(GrammarNode *grammar, NodeID node_id,
                     AstTraversalGenericContext generic_context,
                     void *context) {
  UNUSED(grammar);
  UNUSED(node_id);
  UNUSED(generic_context);
  ASTVerificationContext *ctx = (ASTVerificationContext *)context;
  if (ctx->current_pos[0] != ')') {
    ctx->matches = false;
    snprintf(ctx->error_msg, sizeof(ctx->error_msg), "Expected ')', got: %s",
             ctx->current_pos);
    return AST_TRAVERSAL_STOP;
  }
  ctx->current_pos++;
  // If its not the last sibling, check for a comma
  if (generic_context.sibling_index < generic_context.total_siblings - 1) {
    if (ctx->current_pos[0] != ',') {
      ctx->matches = false;
      snprintf(ctx->error_msg, sizeof(ctx->error_msg), "Expected ',', got: %s",
               ctx->current_pos);
      return AST_TRAVERSAL_STOP;
    }
    ctx->current_pos++;
  }
  return AST_TRAVERSAL_CONTINUE;
}

static AST_TRAVERSAL_ACTION
_verify_token(const Token *token, AstTraversalGenericContext generic_context,
              void *context) {
  UNUSED(token);
  ASTVerificationContext *ctx = (ASTVerificationContext *)context;
  const char *expected_token_str = token_type_to_string(token->type);
  // Check the token type
  if (strncmp(ctx->current_pos, expected_token_str,
              strlen(expected_token_str)) != 0) {
    ctx->matches = false;
    snprintf(ctx->error_msg, sizeof(ctx->error_msg),
             "Expected token type: %s, got: %s", expected_token_str,
             ctx->current_pos);
  }
  // Only if the token has text, check for text in brackets
  if (token->text) {
    if (ctx->current_pos[0] != '(') {
      ctx->matches = false;
      snprintf(ctx->error_msg, sizeof(ctx->error_msg), "Expected '(', got: %s",
               ctx->current_pos);
      return AST_TRAVERSAL_STOP;
    }
    ctx->current_pos++;
    // Check for the text in brackets
    if (strncmp(ctx->current_pos, token->text, strlen(token->text)) != 0) {
      ctx->matches = false;
      snprintf(ctx->error_msg, sizeof(ctx->error_msg),
               "Expected text: %s, got: %s", token->text, ctx->current_pos);
      return AST_TRAVERSAL_STOP;
    }
    ctx->current_pos += strlen(token->text);
    if (ctx->current_pos[0] != ')') {
      ctx->matches = false;
      snprintf(ctx->error_msg, sizeof(ctx->error_msg), "Expected ')', got: %s",
               ctx->current_pos);
      return AST_TRAVERSAL_STOP;
    }
    ctx->current_pos++;
  }
  // Better error message if token doesn't have text, but expected one does
  if (!token->text && ctx->current_pos[0] == '(') {
    ctx->matches = false;
    snprintf(ctx->error_msg, sizeof(ctx->error_msg),
             "Expected text inside token %s, but recieved an empty token",
             expected_token_str);
    return AST_TRAVERSAL_STOP;
  }
  ctx->current_pos += strlen(expected_token_str);
  // If its not the last subling, check for a comma
  if (generic_context.sibling_index < generic_context.total_siblings - 1) {
    if (ctx->current_pos[0] != ',') {
      ctx->matches = false;
      snprintf(ctx->error_msg, sizeof(ctx->error_msg), "Expected ',', got: %s",
               ctx->current_pos);
      return AST_TRAVERSAL_STOP;
    }
    ctx->current_pos++;
  }
  return AST_TRAVERSAL_CONTINUE;
}

bool ast_verify_structure(AST *ast, const char *expected_structure) {
  UNUSED(ast);
  UNUSED(expected_structure);
  if (ast_is_empty(ast)) {
    return strlen(expected_structure) == 0;
  }
  AstTraversalVisitor visitor = {
      .visit_token = _verify_token,
      .visit_grammar_enter = _verify_grammar_enter,
      .visit_grammar_exit = _verify_grammar_exit,
  };
  ASTVerificationContext ctx = {
      .expected_pattern = expected_structure,
      .current_pos = expected_structure,
      .matches = true,
      .error_msg = {0},
  };
  ast_traverse(ast, ast_head(*ast), &visitor, &ctx);
  if (!ctx.matches) {
    char *recieved_structure = ast_bracket_print(ast);
    DZ_ERROR("AST does not match expected structure: %s\n\t\tExpected: "
             "%s\n\t\tGot:      %s\n",
             ctx.error_msg, expected_structure, recieved_structure);
    free(recieved_structure);
    DZ_DEBUGBREAK();
  }
  return ctx.matches;
}