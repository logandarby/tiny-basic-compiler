#include "parser.h"

// ---------------------------
// PARSER PARSE
//
// This file contains the implementation of the main parser function.
// ---------------------------

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

bool pc_match(ParseContext *pc, const enum TOKEN type) {
  const Token token = pc_peek(pc);
  return token.type == type;
}

bool pc_match_array(ParseContext *pc, const enum TOKEN types[],
                    const size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (pc_match(pc, types[i])) {
      return true;
    }
  }
  return false;
}

bool pc_done(ParseContext *pc) {
  return pc->_position >= token_array_length(pc->_ta);
}

void pc_add_token_and_advance(ParseContext *pc, AST *ast, NodeID parent_node) {
  ast_node_add_child_token(ast, parent_node, pc_peek(pc));
  pc_next(pc);
}

// ====================
// PARSER IMPLEMENTATION
// ====================

bool _parse_primary(AST *ast, NodeID parent_node, ParseContext *pc) {
  if (token_array_is_empty(pc->_ta)) {
    return false;
  }
  const NodeID primary_node =
      ast_node_add_child_grammar(ast, parent_node, GRAMMAR_TYPE_PRIMARY);
  if (pc_match(pc, TOKEN_NUMBER)) {
    pc_add_token_and_advance(pc, ast, primary_node);
  } else if (pc_match(pc, TOKEN_IDENT)) {
    pc_add_token_and_advance(pc, ast, primary_node);
  } else {
    return false;
  }
  return true;
}

bool _parse_unary(AST *ast, NodeID parent_node, ParseContext *pc) {
  if (token_array_is_empty(pc->_ta)) {
    return false;
  }
  const NodeID unary_node =
      ast_node_add_child_grammar(ast, parent_node, GRAMMAR_TYPE_UNARY);
  if (pc_match_array(pc, (const enum TOKEN[]){TOKEN_PLUS, TOKEN_MINUS}, 2)) {
    pc_add_token_and_advance(pc, ast, unary_node);
    return _parse_primary(ast, unary_node, pc);
  } else {
    return _parse_primary(ast, unary_node, pc);
  }
  return false;
}

bool _parse_term(AST *ast, NodeID parent_node, ParseContext *pc) {
  if (token_array_is_empty(pc->_ta)) {
    return false;
  }
  const NodeID term_node =
      ast_node_add_child_grammar(ast, parent_node, GRAMMAR_TYPE_TERM);
  while (true) {
    if (_parse_unary(ast, term_node, pc)) {
      if (pc_match_array(pc, (const enum TOKEN[]){TOKEN_DIV, TOKEN_MULT}, 2)) {
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
  if (token_array_is_empty(pc->_ta)) {
    return false;
  }
  const NodeID expression_node =
      ast_node_add_child_grammar(ast, parent_node, GRAMMAR_TYPE_EXPRESSION);
  while (true) {
    if (_parse_term(ast, expression_node, pc)) {
      if (pc_match_array(pc, (const enum TOKEN[]){TOKEN_PLUS, TOKEN_MINUS},
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

bool _parse_comparison(AST *ast, NodeID parent_node, ParseContext *pc) {
  if (token_array_is_empty(pc->_ta)) {
    return false;
  }
  const NodeID comparison_node =
      ast_node_add_child_grammar(ast, parent_node, GRAMMAR_TYPE_COMPARAISON);
  // TODO: This should be + instead of *
  while (true) {
    if (_parse_expression(ast, comparison_node, pc)) {
      if (pc_match_array(pc,
                         (const enum TOKEN[]){TOKEN_EQEQ, TOKEN_NOTEQ, TOKEN_GT,
                                              TOKEN_LT, TOKEN_GTE, TOKEN_LTE},
                         6)) {
        pc_add_token_and_advance(pc, ast, comparison_node);
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

// Parses a statement.
// Returns true if the statement was parsed successfully, false otherwise.
bool _parse_statement(AST *ast, NodeID parent_node, ParseContext *pc) {
  // For now, only implementing PRINT
  if (token_array_is_empty(pc->_ta)) {
    return false;
  }
  const NodeID statement_node =
      ast_node_add_child_grammar(ast, parent_node, GRAMMAR_TYPE_STATEMENT);
  if (pc_match(pc, TOKEN_PRINT)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (pc_match(pc, TOKEN_STRING)) {
      pc_add_token_and_advance(pc, ast, statement_node);
      return true;
    } else {
      _parse_expression(ast, statement_node, pc);
      return true;
    }
  } else if (pc_match(pc, TOKEN_IF)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!_parse_comparison(ast, statement_node, pc)) {
      return false;
    }
    if (!pc_match(pc, TOKEN_THEN)) {
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!_parse_statement(ast, statement_node, pc)) {
      return false;
    }
    if (!pc_match(pc, TOKEN_ENDIF)) {
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    return true;
  } else if (pc_match(pc, TOKEN_WHILE)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!_parse_comparison(ast, statement_node, pc)) {
      return false;
    }
    if (!pc_match(pc, TOKEN_REPEAT)) {
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!_parse_statement(ast, statement_node, pc)) {
      return false;
    }
    if (!pc_match(pc, TOKEN_ENDWHILE)) {
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    return true;
  } else if (pc_match(pc, TOKEN_LABEL)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!pc_match(pc, TOKEN_IDENT)) {
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    return true;
  } else if (pc_match(pc, TOKEN_GOTO)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!pc_match(pc, TOKEN_IDENT)) {
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    return true;
  } else if (pc_match(pc, TOKEN_LET)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!pc_match(pc, TOKEN_IDENT)) {
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!pc_match(pc, TOKEN_EQ)) {
      return false;
    }
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!_parse_expression(ast, statement_node, pc)) {
      return false;
    }
    return true;
  } else if (pc_match(pc, TOKEN_INPUT)) {
    pc_add_token_and_advance(pc, ast, statement_node);
    if (!pc_match(pc, TOKEN_IDENT)) {
      return false;
    }
    return true;
  }
  return false;
}

// Parses program ::= {statement}
// Returns true if the program was parsed successfully, false otherwise.
bool _parse_program(AST *ast, NodeID parent_node, ParseContext *pc) {
  while (!pc_done(pc)) {
    if (!_parse_statement(ast, parent_node, pc)) {
      return false;
    }
  }
  return true;
}

AST ast_parse(const TokenArray ta) {
  AST ast = ast_init();
  if (token_array_is_empty(ta)) {
    return ast;
  }
  ast_create_root_node(&ast, GRAMMAR_TYPE_PROGRAM);
  ParseContext pc = pc_init(ta);
  _parse_program(&ast, ast_head(ast), &pc);
  return ast;
}