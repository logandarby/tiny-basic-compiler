#pragma once

// ====================
// AST UTILS
//
// These are AST utils for testing, debugging, and printing.
// ====================

#include "ast.h"

// Prints the AST. An example of the traversal pattern.
void ast_print(AST *ast);

// A testing utility used to verify the structure of the AST.
// Example:
// ast_verify_structure(&ast,
//  "PROGRAM(STATEMENT(LET,IDENT(x),EQ,EXPRESSION(NUMBER(5),PLUS,NUMBER(3))))"
// );
bool ast_verify_structure(AST *ast, const char *expected_structure);