#pragma once

// ---------------------------------------------
// PARSER
//
// Given a list of Tokens from the Lexer, the Parser turns
// the list of tokens into an AST based off the Tiny BASIC grammar
// ---------------------------------------------

#include "../../ast/ast.h"
#include "../lexer/token.h"

/*
GRAMMAR DEFINITION

program ::= {statement}*
statement ::= "PRINT" (expression | string) nl
    | "IF" comparison "THEN" nl {statement}* "ENDIF" nl
    | "WHILE" comparison "REPEAT" nl {statement}* "ENDWHILE" nl
    | "LABEL" ident nl
    | "GOTO" ident nl
    | "LET" ident "=" expression nl
    | "INPUT" ident nl
comparison ::= expression ("==" | "!=" | ">" | ">=" | "<" | "<=") expression
expression ::= term {( "-" | "+" ) term}
term ::= unary {( "/" | "*" ) unary}
unary ::= ["+" | "-"] primary
primary ::= number | ident
nl ::= '\n'+

comment ::= "REM" {.}* nl
Comments are ignored.
*/

// Initializes an AST and parses the TokenArray according to the
// grammar rules above.
// The AST must be destroyed with the ast_destroy function
AST ast_parse(const TokenArray ta);
