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
  TOKEN_IDENT,
  // Internal Keywords
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
};

typedef struct {
  enum TOKEN type;
  char *text; // Optionally stores the actual text for this token (numbers,
              // identifiers, strings, etc.)
} Token;

typedef struct TokenArrayHandle *TokenArray;

// ------------------------------------
// TOKEN Struct API
// ------------------------------------

Token token_create(TokenArray ta, enum TOKEN type, const char *text,
                   size_t length);
Token token_create_simple(enum TOKEN type);

bool token_is_number(const Token token);
bool token_is_string(const Token token);
bool token_is_identifier(const Token token);
bool token_is_keyword(const Token token);
bool token_is_operator(const Token token);

// Destroys any allocated data associated with the Token
void token_destroy(Token token);

// ------------------------------------
// TOKEN ARRAY UTIL
// Dynamic array for storing tokens
// ------------------------------------

TokenArray token_array_init(void);

// Push a token with text content
void token_array_push(TokenArray ta, enum TOKEN token_type, const char *text,
                      size_t length);
// Pushes a string token with string content
// First cleans the string of any escaped characters. For example, if the string
// 'Hello \"quotes\"' is pushed with the delimiter={"}, then the string will be
// cleaned to 'Hello "quotes"'.
void token_array_clean_and_push_string(TokenArray ta, const char *text,
                                       size_t length);
// Push a simple token (operators, keywords, etc.) with no text content
void token_array_push_simple(TokenArray ta, enum TOKEN token_type);

// Returns the length of the TokenArray
size_t token_array_length(const TokenArray ta);

// Returns the capacity of the TokenArray
size_t token_array_capacity(const TokenArray ta);

// Returns if the array is empty
bool token_array_is_empty(const TokenArray ta);

// Returns token at a location
Token token_array_at(const TokenArray ta, size_t index);

// Destroys a TokenArray, all the tokens within it, and sets the pointer to NULL
void token_array_destroy(TokenArray *ta);

// ------------------------------------
// LEXER API
// ------------------------------------

// Parses from a file reader, and returns the result as a dynamically
// allocated TokenArray. You must call token_array_destroy
TokenArray lexer_parse(FileReader filereader);
