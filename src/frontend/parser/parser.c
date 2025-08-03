#include "parser.h"
#include "../../common/error_reporter.h"
#include "compiler.h"
#include "dz_debug.h"
#include "token.h"
#include <stdarg.h>
#include <string.h>

// ---------------------------
// PARSER PARSE
//
// This file contains the implementation of the main parser function.
// ---------------------------

// Keywords that signal the start of a statement
static const enum TOKEN STATEMENT_START_KEYWORDS[] = {
    TOKEN_PRINT, TOKEN_IF,  TOKEN_WHILE, TOKEN_LABEL,
    TOKEN_GOTO,  TOKEN_LET, TOKEN_INPUT};
static const enum TOKEN CONTROL_FLOW_TOKENS[] = {TOKEN_ENDIF, TOKEN_ENDWHILE,
                                                 TOKEN_ELSE};

// ====================
// PARSE CONTEXT
// ====================

// Meant to be an opaque-ish struct used to pass around context to the parser.
// Preferably, the struct methods should be used to operate on it
typedef struct {
  TokenArray _ta;
  size_t _position;
} ParseContext;

ParseContext pc_init(const TokenArray ta) {
  ParseContext pc;
  pc._ta = ta;
  pc._position = 0;
  return pc;
}

bool pc_done(ParseContext *pc) {
  return pc->_position >= token_array_length(pc->_ta);
}

ParseContext pc_next(ParseContext *pc) {
  pc->_position++;
  return *pc;
}

Token pc_peek(ParseContext *pc) {
  return token_array_at(pc->_ta, pc->_position);
}

Token pc_peek_next(ParseContext *pc) {
  pc_next(pc);
  return token_array_at(pc->_ta, pc->_position);
}

bool pc_expect(ParseContext *pc, const enum TOKEN type) {
  if (pc_done(pc))
    return false;
  const Token token = pc_peek(pc);
  return token.type == type;
}

bool pc_expect_array(ParseContext *pc, const enum TOKEN types[],
                     const size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (pc_expect(pc, types[i])) {
      return true;
    }
  }
  return false;
}

void pc_add_token_and_advance(ParseContext *pc, AST *ast, NodeID parent_node) {
  ast_node_add_child_token(ast, parent_node, pc_peek(pc));
  pc_next(pc);
}

// ====================
// PARSER IMPLEMENTATION
// ====================

void _recover_to_next_statement(ParseContext *pc) {
  while (!pc_done(pc)) {
    if (pc_expect_array(pc, STATEMENT_START_KEYWORDS,
                        array_size(STATEMENT_START_KEYWORDS)))
      return;
    pc_next(pc);
  }
}

bool _parse_primary(AST *ast, NodeID parent_node, ParseContext *pc) {
  if (pc_done(pc)) {
    return false;
  }
  const NodeID primary_node =
      ast_node_add_child_grammar(ast, parent_node, GRAMMAR_TYPE_PRIMARY);
  if (pc_expect(pc, TOKEN_NUMBER)) {
    pc_add_token_and_advance(pc, ast, primary_node);
  } else if (pc_expect(pc, TOKEN_IDENT)) {
    pc_add_token_and_advance(pc, ast, primary_node);
  } else {
    return false;
  }
  return true;
}

bool _parse_unary(AST *ast, NodeID parent_node, ParseContext *pc) {
  if (pc_done(pc)) {
    return false;
  }
  const NodeID unary_node =
      ast_node_add_child_grammar(ast, parent_node, GRAMMAR_TYPE_UNARY);
  if (pc_expect_array(pc, (const enum TOKEN[]){TOKEN_PLUS, TOKEN_MINUS}, 2)) {
    pc_add_token_and_advance(pc, ast, unary_node);
    return _parse_primary(ast, unary_node, pc);
  } else {
    return _parse_primary(ast, unary_node, pc);
  }
  return false;
}

bool _parse_term(AST *ast, NodeID parent_node, ParseContext *pc) {
  if (pc_done(pc)) {
    return false;
  }
  const NodeID term_node =
      ast_node_add_child_grammar(ast, parent_node, GRAMMAR_TYPE_TERM);
  while (true) {
    if (_parse_unary(ast, term_node, pc)) {
      if (pc_expect_array(pc, (const enum TOKEN[]){TOKEN_DIV, TOKEN_MULT}, 2)) {
        pc_add_token_and_advance(pc, ast, term_node);
        continue;
      } else {
        break;
      }
    } else {
      return false;
    }
  }
  return true;
}

bool _parse_expression(AST *ast, NodeID parent_node, ParseContext *pc) {
  if (pc_done(pc)) {
    return false;
  }
  const NodeID expression_node =
      ast_node_add_child_grammar(ast, parent_node, GRAMMAR_TYPE_EXPRESSION);
  while (true) {
    if (_parse_term(ast, expression_node, pc)) {
      if (pc_expect_array(pc, (const enum TOKEN[]){TOKEN_PLUS, TOKEN_MINUS},
                          2)) {
        pc_add_token_and_advance(pc, ast, expression_node);
        continue;
      } else {
        break;
      }
    } else {
      return false;
    }
  }
  return true;
}

static const enum TOKEN COMPARISON_OPS[] = {TOKEN_EQEQ, TOKEN_NOTEQ, TOKEN_GT,
                                            TOKEN_LT,   TOKEN_GTE,   TOKEN_LTE};

bool _parse_comparison(AST *ast, NodeID parent_node, ParseContext *pc) {
  if (pc_done(pc)) {
    return false;
  }
  const NodeID comparison_node =
      ast_node_add_child_grammar(ast, parent_node, GRAMMAR_TYPE_COMPARISON);
  if (!_parse_expression(ast, comparison_node, pc))
    return false;
  if (!pc_expect_array(pc, COMPARISON_OPS, array_size(COMPARISON_OPS)))
    return false;
  pc_add_token_and_advance(pc, ast, comparison_node);
  if (!_parse_expression(ast, comparison_node, pc))
    return false;
  return true;
}

static bool _parse_statement_star_internal(AST *ast, NodeID parent_node,
                                           ParseContext *pc, bool inside_block);
bool _parse_statement_star(AST *ast, NodeID parent_node, ParseContext *pc);

void pc_error_current_token(ParseContext *pc, const char *msg, ...)
    FORMAT_PRINTF(2, 3);
void pc_error_current_token(ParseContext *pc, const char *msg, ...) {
  FileLocation file_pos = {1, 1}; // Default location
  if (!pc_done(pc)) {
    file_pos = token_get_file_pos(pc_peek(pc));
  }
  va_list args;
  va_start(args, msg);
  er_add_error_v(ERROR_GRAMMAR, "parser.c", file_pos.line, file_pos.col, msg,
                 args);
  va_end(args);
}

// Parses a statement.
// Returns true if the statement was parsed successfully, false otherwise.
bool _parse_statement(AST *ast, NodeID parent_node, ParseContext *pc) {
  if (pc_done(pc)) {
    pc_error_current_token(
        pc, "Expected a statement, but instead reached the end of file.");
    return false;
  }
  const NodeID statement_node =
      ast_node_add_child_grammar(ast, parent_node, GRAMMAR_TYPE_STATEMENT);
  // "PRINT" (expression | string) nl
  if (pc_expect(pc, TOKEN_PRINT)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (pc_expect(pc, TOKEN_STRING)) {
      pc_add_token_and_advance(pc, ast, statement_node);
      return true;
    } else {
      const bool expr_success = _parse_expression(ast, statement_node, pc);
      if (!expr_success) {
        pc_error_current_token(pc, "Expected expression after token PRINT");
      }
      return expr_success;
    }
    // "IF" comparison "THEN" nl {statement}* "ENDIF" nl
  } else if (pc_expect(pc, TOKEN_IF)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!_parse_comparison(ast, statement_node, pc)) {
      pc_error_current_token(pc, "IF statement must contain a comparison.");
      return false;
    }
    if (!pc_expect(pc, TOKEN_THEN)) {
      pc_error_current_token(
          pc, "Expected THEN keyword in IF statement. IF statement must take "
              "the form \"IF <comparison> THEN\"...");
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!_parse_statement_star_internal(ast, statement_node, pc, true)) {
      pc_error_current_token(pc, "IF statement does not contain a proper body! "
                                 "Please fix any errors inside it.");
      return false;
    }
    if (!pc_expect(pc, TOKEN_ENDIF)) {
      pc_error_current_token(pc, "IF statements must end with an ENDIF");
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    return true;
    // "WHILE" comparison "REPEAT" nl {statement}* "ENDWHILE" nl
  } else if (pc_expect(pc, TOKEN_WHILE)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!_parse_comparison(ast, statement_node, pc)) {
      pc_error_current_token(
          pc, "WHILE statement must contain a valid comparison.");
      return false;
    }
    if (!pc_expect(pc, TOKEN_REPEAT)) {
      pc_error_current_token(
          pc, "Expected REPEAT keyword in WHILE statement. WHILE statement "
              "must take the form \"WHILE <comparison> REPEAT\"...");
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!_parse_statement_star_internal(ast, statement_node, pc, true)) {
      pc_error_current_token(pc, "WHILE statement does not contain a proper "
                                 "body! Please fix any errors inside it.");
      return false;
    }
    if (!pc_expect(pc, TOKEN_ENDWHILE)) {
      pc_error_current_token(pc, "WHILE statements must end with an ENDWHILE");
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    return true;
    // "LABEL" ident nl
  } else if (pc_expect(pc, TOKEN_LABEL)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!pc_expect(pc, TOKEN_IDENT)) {
      pc_error_current_token(pc, "Expected an identifier after LABEL keyword");
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    return true;
    // "GOTO" ident nl
  } else if (pc_expect(pc, TOKEN_GOTO)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!pc_expect(pc, TOKEN_IDENT)) {
      pc_error_current_token(pc, "Expected an identifier after GOTO keyword");
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    return true;
    // "LET" ident "=" expression nl
  } else if (pc_expect(pc, TOKEN_LET)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!pc_expect(pc, TOKEN_IDENT)) {
      pc_error_current_token(pc, "Expected a variable name after LET keyword");
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!pc_expect(pc, TOKEN_EQ)) {
      pc_error_current_token(
          pc, "Expected \"=\" after variable name in LET statement");
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!_parse_expression(ast, statement_node, pc)) {
      pc_error_current_token(
          pc, "Expected an expression after \"=\" in LET statement");
      return false;
    }
    return true;
    // "INPUT" ident nl
  } else if (pc_expect(pc, TOKEN_INPUT)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!pc_expect(pc, TOKEN_IDENT)) {
      pc_error_current_token(pc,
                             "Expected a variable name after INPUT keyword");
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    return true;
  }

  // ERROR: Statement does not start with correct token
  // Build dynamic error message with all statement keywords
  char error_msg[256] = "Unknown statement. Expected one of: ";
  size_t remaining = sizeof(error_msg) - strlen(error_msg) - 1;
  for (size_t i = 0; i < array_size(STATEMENT_START_KEYWORDS); i++) {
    if (i > 0 && remaining > 2) {
      strncat(error_msg, ", ", remaining);
      remaining -= 2;
    }
    const char *keyword = token_type_to_string(STATEMENT_START_KEYWORDS[i]);
    size_t keyword_len = strlen(keyword);
    if (remaining > keyword_len) {
      strncat(error_msg, keyword, remaining);
      remaining -= keyword_len;
    }
  }

  pc_error_current_token(pc, "%s", error_msg);
  return false;
}

// Must free string after
char *get_unknown_statement_err_msg(void) {
  char error_msg[256] = "Unknown statement. Expected one of: ";
  size_t remaining = sizeof(error_msg) - strlen(error_msg) - 1;
  for (size_t i = 0; i < array_size(STATEMENT_START_KEYWORDS); i++) {
    if (i > 0 && remaining > 2) {
      strncat(error_msg, ", ", remaining);
      remaining -= 2;
    }
    const char *keyword = token_type_to_string(STATEMENT_START_KEYWORDS[i]);
    size_t keyword_len = strlen(keyword);
    if (remaining > keyword_len) {
      strncat(error_msg, keyword, remaining);
      remaining -= keyword_len;
    }
  }
  return strdup(error_msg);
}

// Parses 0 or more statements
// If inside_block is true, control flow tokens (ENDIF, ENDWHILE) are treated as
// valid end markers If inside_block is false, they are treated as unknown
// statements and generate errors
static bool _parse_statement_star_internal(AST *ast, NodeID parent_node,
                                           ParseContext *pc,
                                           bool inside_block) {
  while (!pc_done(pc)) {
    const bool is_keyword = pc_expect_array(
        pc, STATEMENT_START_KEYWORDS, array_size(STATEMENT_START_KEYWORDS));
    // If we encounter a token that's not a statement keyword, check if it's a
    // control flow token
    if (!is_keyword) {
      // Check if it's a valid end-of-block token (ENDIF, ENDWHILE, ELSE, etc.)
      const bool is_control_flow = pc_expect_array(
          pc, CONTROL_FLOW_TOKENS, array_size(CONTROL_FLOW_TOKENS));

      if (is_control_flow && inside_block) {
        // This is expected - end of the current statement block
        return true;
      } else {
        // This is an unknown statement - report error
        char *error_msg = get_unknown_statement_err_msg();
        pc_error_current_token(pc, "%s", error_msg);
        free(error_msg);
        // Recover to next statement and continue
        _recover_to_next_statement(pc);
        continue;
      }
    }
    // Expect a statement
    if (!_parse_statement(ast, parent_node, pc)) {
      // Errors are reported on individual functions
      // Recover to next statement, and continue
      _recover_to_next_statement(pc);
      continue;
    }
  }
  return true;
}

// Public wrapper for statement parsing (top-level)
bool _parse_statement_star(AST *ast, NodeID parent_node, ParseContext *pc) {
  return _parse_statement_star_internal(ast, parent_node, pc, false);
}

// Parses program ::= {statement}
// Returns true if the program was parsed successfully, false otherwise.
bool _parse_program(AST *ast, NodeID parent_node, ParseContext *pc) {
  return _parse_statement_star(ast, parent_node, pc);
}

AST ast_parse(const TokenArray ta) {
  AST ast = ast_init();
  ast_create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);
  if (token_array_is_empty(ta)) {
    return ast;
  }
  ParseContext pc = pc_init(ta);
  _parse_program(&ast, ast_head(ast), &pc);
  return ast;
}
