#pragma once

// -----------------------------
// x86 EMITTER
//
// Takes an AST with symbol tables, etc. and emits
// x86 assembly
// -----------------------------

#include "../ast/ast.h"
#include "../common/symbol_table.h"

// Emits x86 assembly to the given file given an AST and several tables
void emit_x86(HostInfo host, FILE *file, AST *ast, VariableTable *table);
