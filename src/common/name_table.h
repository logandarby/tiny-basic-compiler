#pragma once

// -----------------------------------
// SYMBOL TABLE
//
// A dictionary of symbols. Maps an identifier string to
// an entry which contains info about the variable. Analyzes
// the type of the variable as well
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

typedef enum {
  IDENTIFIER_UNKNOWN,
  IDENTIFIER_VARIABLE,
  IDENTIFIER_LABEL,
} IDENTIFIER_TYPE;

typedef struct IdentifierInfo {
  FileLocation file_pos;
  IDENTIFIER_TYPE type;
} IdentifierInfo;

typedef struct IdentifierHash {
  char *key;
  IdentifierInfo value;
} IdentifierHash;

typedef IdentifierHash *IdentifierTable;

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

// -----------
// API
// Builds Several name tables from the AST
// Used for debug printing, and for semantic analysis
// Only captures the first declaration of any type. If there are duplicates, the
// first in the file is put into the table
// -----------

typedef struct {
  LiteralTable literal_table;
  IdentifierTable identifier_table;
} NameTable;

// Gets all string literals and all integer symbols from the ast
// Must vall variables_destroy after
NameTable *name_table_collect_from_ast(AST *ast);
void name_table_destroy(NameTable *var_table);
