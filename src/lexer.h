#pragma once
#include "core.h"
#include "file.h"

// ------------------------------------
// LEXER
//
// Give a FileReader, lexes the input into a list of tokens
// ------------------------------------

#define OPERATOR_START 100
#define LITERAL_START 200
#define KEYWORD_START 300

enum TOKEN {
  // Unknown
  TOKEN_UNKNOWN = 0,
  // Operators
  TOKEN_PLUS = OPERATOR_START,
  TOKEN_MINUS,
  TOKEN_MULT,
  TOKEN_DIV,
  // Relation Operators
  TOKEN_GT,
  TOKEN_LT,
  TOKEN_GTE,
  TOKEN_LTE,
  TOKEN_EQ,
  TOKEN_NOTEQ,
  TOKEN_EQEQ,
  // Logic Operators
  TOKEN_NOT,
  TOKEN_AND,
  TOKEN_OR,
  // Literals
  TOKEN_STRING = LITERAL_START,
  TOKEN_NUMBER,
  TOKEN_ENDOFFILE,
  TOKEN_NEWLINE,
  TOKEN_IDENT,
  // Internal Keywords
  TOKEN_LABEL = KEYWORD_START,
  TOKEN_GOTO,
  TOKEN_PRINT,
  TOKEN_INPUT,
  TOKEN_LET,
  TOKEN_IF,
  TOKEN_THEN,
  TOKEN_ELSE,
  TOKEN_ENDIF,
  TOKEN_WHILE,
  TOKEN_REPEAT,
  TOKEN_ENDWHILE,
};

// ------------------------------------
// TOKEN ARRAY UTIL
// Dynamic array for storing tokens
// ------------------------------------

typedef struct TokenArrayHandle *TokenArray;

TokenArray token_array_init(void);

void token_array_push(TokenArray ta, enum TOKEN token);

// Returns the length of the TokenArray
size_t token_array_length(const TokenArray ta);

// Returns the capacity of the TokenArray
size_t token_array_capacity(const TokenArray ta);

// Returns if the array is empty
bool token_array_is_empty(const TokenArray ta);

// Returns token at a location
enum TOKEN token_array_at(const TokenArray ta, size_t index);

// Destroys a TokenArray and sets the pointer to NULL
void token_array_destroy(TokenArray *ta);

// ------------------------------------
// LEXER API
// ------------------------------------

// Parses from a file reader, and returns the result as a dynamically
// allocated TokenArray. You must call token_array_destroy
TokenArray lexer_parse(FileReader filereader);
