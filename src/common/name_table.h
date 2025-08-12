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
// For variables
// -----------

typedef struct SymbolInfo {
  FileLocation file_pos;
} SymbolInfo;

typedef struct SymbolHash {
  char *key;
  SymbolInfo value;
} SymbolHash;

typedef SymbolHash *SymbolTable;

// -----------
// Literal Table
// Maps string literal
// -----------

typedef struct LiteralInfo {
  uint32_t
      label; // Label is an integer, but will be translated to ".L<int>" in asm
  FileLocation file_pos;
} LiteralInfo;

typedef struct LiteralHash {
  char *key; // Keyed by the string value. Supports interning
  LiteralInfo value;
} LiteralHash;

typedef LiteralHash *LiteralTable;

// ------------
// Labels
// Maps label name to value
// ------------

typedef struct {
  FileLocation file_pos;
} LabelInfo;

typedef struct {
  char *key; // keyed by label name
  LabelInfo value;
} LabelHash;

typedef LabelHash *LabelTable;

// -----------
// API
// Builds a LiteralTable and SymbolTable from an ast
// -----------

typedef struct {
  LiteralTable literal_table;
  SymbolTable symbol_table;
  LabelTable label_table;
} NameTable;

// Gets all string literals and all integer symbols from the ast
// Must vall variables_destroy after
NameTable *name_table_collect_from_ast(AST *ast);
void name_table_destroy(NameTable *var_table);
