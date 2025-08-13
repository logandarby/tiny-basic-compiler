#pragma once

#include "../../core/core.h"

// Token categorization system: Uses explicit enum values with fixed spacing
// to enable O(1) category checking via range comparisons.
// Each category occupies a range of TOKEN_CATEGORY_SPACING values.
#define TOKEN_CATEGORY_SPACING 100
#define OPERATOR_START (TOKEN_CATEGORY_SPACING) // Range: 100-199
#define LITERAL_START                                                          \
  (OPERATOR_START + TOKEN_CATEGORY_SPACING)                    // Range: 200-299
#define KEYWORD_START (LITERAL_START + TOKEN_CATEGORY_SPACING) // Range: 300-399

enum TOKEN {
  // Unknown
  TOKEN_UNKNOWN = 0,

  // Operators: Arithmetic, relational, and logical operators
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

  // Literals: String, number, and identifier tokens
  TOKEN_STRING = LITERAL_START,
  TOKEN_NUMBER,
  TOKEN_IDENT,

  // Keywords: Language-specific reserved words
  TOKEN_LABEL = KEYWORD_START,
  TOKEN_PRINT,
  TOKEN_INPUT,
  TOKEN_LET,
  TOKEN_IF,
  TOKEN_GOTO,
  TOKEN_THEN,
  TOKEN_ELSE,
  TOKEN_ENDIF,
  TOKEN_WHILE,
  TOKEN_REPEAT,
  TOKEN_ENDWHILE,
  TOKEN_REM,
};

typedef struct FileLocation {
  uint32_t line;
  uint32_t col;
} FileLocation;

typedef struct {
  enum TOKEN type;
  char *text; // Optionally stores the actual text for this token (numbers,
              // identifiers, strings, etc.)
  struct FileLocation file_pos; // Stores where in the file the token is located
                                // for better error reporting
} Token;

typedef struct TokenArrayHandle *TokenArray;

// ------------------------------------
// TOKEN Struct API
// ------------------------------------

// Create token. Must call token_destroy afterwards
Token token_create(TokenArray ta, enum TOKEN type, const char *text,
                   uint32_t length, FileLocation location);
// Create token. Must call token_destroy afterwards
Token token_create_simple(enum TOKEN type, FileLocation location);

// Get information about token type
bool token_is_number(const Token token);
bool token_is_string(const Token token);
bool token_is_identifier(const Token token);
bool token_is_keyword(const Token token);
bool token_is_operator(const Token token);

// Get the location where the token appears in the file
FileLocation token_get_file_pos(const Token token);

// Destroys any allocated data associated with the Token
void token_destroy(Token token);

// A debug function that converts a token type into a legible string.
// Used for debugging purposes, and printing to the console.
const char *token_type_to_string(enum TOKEN type);

// ------------------------------------
// TOKEN ARRAY UTIL
// Dynamic array for storing tokens
// ------------------------------------

TokenArray token_array_init(void);

// Push a token with text content
void token_array_push(TokenArray ta, enum TOKEN token_type, const char *text,
                      uint32_t length, FileLocation location);
// Pushes a string token with string content
// First cleans the string of any escaped characters. For example, if the string
// 'Hello \"quotes\"' is pushed with the delimiter={"}, then the string will be
// cleaned to 'Hello "quotes"'.
void token_array_clean_and_push_string(TokenArray ta, const char *text,
                                       uint32_t length, FileLocation location);
// Push a simple token (operators, keywords, etc.) with no text content
void token_array_push_simple(TokenArray ta, enum TOKEN token_type,
                             FileLocation location);

// Returns the length of the TokenArray
uint32_t token_array_length(const TokenArray ta);

// Returns the capacity of the TokenArray
uint32_t token_array_capacity(const TokenArray ta);

// Returns if the array is empty
bool token_array_is_empty(const TokenArray ta);

// Returns token at a location
Token token_array_at(const TokenArray ta, uint32_t index);

// Destroys a TokenArray, all the tokens within it, and sets the pointer to NULL
void token_array_destroy(TokenArray *ta);
