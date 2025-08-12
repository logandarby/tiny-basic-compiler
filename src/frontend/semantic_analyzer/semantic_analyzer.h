#pragma once

// -------------------------------
// SEMANTIC ANALYZER
//
// Checks for things like variable types being correct,
// Identifiers being defined before use, avoiding duplicate definitions, etc.
// and reports the semantic errors to error_reporter.h
// -------------------------------

/*
 * Checks and reports errors for:
 *  - Inorrect identifier types (GOTO x should have x be a label, not a
 * variable)
 *  - Variable use before definition
 *  - Unknown labels
 *  - Duplicate labels
 */
