#pragma once

// ---------------------------------------------
// PARSER
//
// Given a list of Tokens from the Lexer, the Parser turns
// the list of tokens into an AST based off the Tiny BASIC grammar
// ---------------------------------------------

#include "lexer.h"

/*
GRAMMAR DEFINITION

program ::= {statement}
statement ::= "PRINT" (expression | string) nl
    | "IF" comparison "THEN" nl {statement} "ENDIF" nl
    | "WHILE" comparison "REPEAT" nl {statement} "ENDWHILE" nl
    | "LABEL" ident nl
    | "GOTO" ident nl
    | "LET" ident "=" expression nl
    | "INPUT" ident nl
comparison ::= expression (("==" | "!=" | ">" | ">=" | "<" | "<=") expression)+
expression ::= term {( "-" | "+" ) term}
term ::= unary {( "/" | "*" ) unary}
unary ::= ["+" | "-"] primary
primary ::= number | ident
nl ::= '\n'+
*/

typedef struct ASTNode ASTNode;

#define MAX_AST_CHILDREN 5

typedef enum GRAMMAR_TYPE {
  GRAMMAR_TYPE_PROGRAM,
  GRAMMAR_TYPE_STATEMENT,
  GRAMMAR_TYPE_COMPARAISON,
  GRAMMAR_TYPE_EXPRESSION,
  GRAMMAR_TYPE_TERM,
  GRAMMAR_TYPE_UNARY,
  GRAMMAR_TYPE_PRIMARY,
} GRAMMAR_TYPE;

typedef struct {
  const GRAMMAR_TYPE grammar;
  ASTNode *children[MAX_AST_CHILDREN];
  size_t child_count;
} GrammarNode;

typedef struct {
  ASTNode *_head;
  // ArenaAllocator _allocator;
} AST;

// Buillds an abstract syntax tree based on the grammar specification at the top
// of the file.
AST ast_init(TokenArray ta);
void ast_destroy(AST ast);

bool ast_node_is_token(ASTNode *node);
bool ast_node_is_gramamr(ASTNode *node);

Token ast_node_get_token(ASTNode *node);
GrammarNode ast_node_get_grammer(ASTNode *node);
