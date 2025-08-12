#pragma once

// -------------------------------------
// FRONTEND
//
// Handles parsing the raw BASIC code into tokens,
// Turning it into a syntax tree, and looking for any errors
// -------------------------------------

#include "lexer/lexer.h"
#include "lexer/token.h"
#include "parser/parser.h"
#include "semantic_analyzer/semantic_analyzer.h"
