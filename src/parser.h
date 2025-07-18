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
// Index into out-of-band array with ASTNode struct
typedef size_t NodeID;

#define AST_MAX_CHILDREN 5

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
  GRAMMAR_TYPE grammar;
  NodeID children[AST_MAX_CHILDREN];
  short child_count;
} GrammarNode;

typedef struct {
  NodeID _head;
  // Stores the ASTNodes out of band -- dynamically reallocates
  ASTNode *node_array;
  size_t node_array_capacity;
  size_t node_array_size;
} AST;

// Creates an empty Abstract Syntax Tree
AST ast_init(void);
// Fills an AST with the parsed information from the ta
void ast_parse(AST *ast, const TokenArray ta);
// Creates a root node with the specified grammar type (for testing)
NodeID ast_create_root_node(AST *ast, GRAMMAR_TYPE grammar_type);
NodeID ast_head(AST ast);
void ast_destroy(AST *ast);

bool ast_node_is_token(AST *ast, NodeID node_id);
bool ast_node_is_grammar(AST *ast, NodeID node_id);
// Gets the child of a node. Only works on GrammarNodes-- you should check that
// the node is a grammar node before calling thing
NodeID ast_node_get_child(AST *ast, NodeID parent_id, short child_number);
short ast_node_get_child_count(AST *ast, NodeID node_id);
// These two methods add children to a node. Note that adding children only
// works on GrammarNodes -- you should check this before calling this method
void ast_node_add_child_token(AST *ast, NodeID parent_id, Token token);
void ast_node_add_child_grammar(AST *ast, NodeID parent_id,
                                GRAMMAR_TYPE grammar_type);

const Token *ast_node_get_token(AST *ast, NodeID node_id);
GRAMMAR_TYPE ast_node_get_grammar(AST *ast, NodeID node_id);
