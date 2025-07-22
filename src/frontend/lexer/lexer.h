#pragma once

// ------------------------------------
// LEXER
//
// Give a FileReader, lexes the input into a list of tokens
// ------------------------------------

#include "../../common/file_reader.h"
#include "../../core/core.h"
#include "token.h"

// ------------------------------------
// LEXER API
// ------------------------------------

// Parses from a file reader, and returns the result as a dynamically
// allocated TokenArray. You must call token_array_destroy
TokenArray lexer_parse(FileReader filereader);
