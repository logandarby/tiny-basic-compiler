#pragma once

// -----------------------------------
// SYMBOL TABLE
//
// A dictionary of symbols. Maps an identifier string to
// an entry which contains the variable (always integers)
//
// Also contains logic to collect string literals
// -----------------------------------

#include "../ast/ast.h"
#include "../core/core.h"
#include "token.h"

// -----------
// Symbol Table
// -----------

typedef struct SymbolInfo {
  size_t
      label; // Label is an integer, but will be translated to ".L<int>" in asm
} SymbolInfo;

typedef struct SymbolHash {
  char *key;
  SymbolInfo value;
} SymbolHash;

typedef SymbolHash *SymbolTable;

// -----------
// Literal Table
// Maps filelocation to string literal
// -----------

typedef struct LiteralInfo {
  size_t
      label; // Label is an integer, but will be translated to ".L<int>" in asm
} LiteralInfo;

typedef struct LiteralHash {
  char *key; // Keyed by the string value. Supports interning
  LiteralInfo value;
} LiteralHash;

typedef LiteralHash *LiteralTable;

// -----------
// API
// Builds a LiteralTable and SymbolTable from an ast
// -----------

typedef struct {
  LiteralTable literal_table;
  SymbolTable symbol_table;
} VariableTable;

// Gets all string literals and all integer symbols from the ast
// Must vall variables_destroy after
VariableTable *variables_collect_from_ast(AST *ast);
void variables_destroy(VariableTable *var_table);
