#pragma once

// -------------------------------
// SEMANTIC ANALYZER
//
// Checks for things like variable types being correct,
// Identifiers being defined before use, avoiding duplicate definitions, etc.
// and reports the semantic errors to error_reporter.h
// -------------------------------

#include "../../ast/ast.h"
#include "../../common/name_table.h"

/*
 * Checks and reports errors for:
 *  - Inorrect identifier types (GOTO x should have x be a label, not a
 * variable)
 *  - Variable use before definition
 *  - Unknown labels
 *  - Duplicate labels
 * Returns if it is successful
 */
bool semantic_analyzer_check(AST *ast, NameTable *table);
