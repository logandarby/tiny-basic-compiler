#include "lexer.h"
#include "core.h"
#include "file.h"
#include <string.h>

const size_t INIT_CAPACITY = 512;
const unsigned int CAPACITY_MULTIPLIER = 2;

// ------------------------------------
// Tokens Implementation
// ------------------------------------

Token token_create(enum TOKEN type, const char *text, size_t length) {
  Token t = {.type = type, .text = NULL};
  if (text && length > 0) {
    t.text = xmalloc(length + 1);
    strncpy(t.text, text, length);
    t.text[length] = '\0';
  }
  return t;
}

Token token_create_simple(enum TOKEN type) {
  Token t = {.type = type, .text = NULL};
  return t;
}

bool token_is_number(const Token token) { return token.type == TOKEN_NUMBER; }
bool token_is_string(const Token token) { return token.type == TOKEN_STRING; }
bool token_is_identifier(const Token token) {
  return token.type == TOKEN_IDENT;
}
bool token_is_keyword(const Token token) { return token.type >= KEYWORD_START; }
bool token_is_operator(const Token token) {
  return token.type >= OPERATOR_START && token.type <= LITERAL_START;
}

// Destroys any allocated data associated with the Token
void token_destroy(Token token) {
  if (token.text) {
    free(token.text);
  }
}

// ------------------------------------
// Token Array Implementation
// ------------------------------------

struct TokenArrayHandle {
  Token *head;
  size_t size;     // # of elements stored
  size_t capacity; // Total Capacity
};

void _resize_token_array(TokenArray ta, const size_t new_size) {
  ta->head = xrealloc(ta->head, new_size * sizeof(Token));
  ta->capacity = new_size;
}

TokenArray token_array_init(void) {
  struct TokenArrayHandle ta = {
      .head = xmalloc(sizeof(Token) * INIT_CAPACITY),
      .size = 0,
      .capacity = INIT_CAPACITY,
  };
  TokenArray return_val = xmalloc(sizeof(struct TokenArrayHandle));
  memcpy(return_val, &ta, sizeof(struct TokenArrayHandle));
  return return_val;
}

void token_array_push_simple(TokenArray ta, enum TOKEN token_type) {
  if (ta->size == ta->capacity) {
    _resize_token_array(ta, ta->capacity * CAPACITY_MULTIPLIER);
  }
  ta->head[ta->size] = token_create_simple(token_type);
  ta->size++;
}

void token_array_push(TokenArray ta, enum TOKEN token_type, const char *text,
                      size_t length) {
  if (ta->size == ta->capacity) {
    _resize_token_array(ta, ta->capacity * CAPACITY_MULTIPLIER);
  }
  ta->head[ta->size] = token_create(token_type, text, length);
  ta->size++;
}

size_t token_array_length(const TokenArray ta) { return ta->size; }

size_t token_array_capacity(const TokenArray ta) { return ta->capacity; }

bool token_array_is_empty(const TokenArray ta) { return ta->size == 0; }

Token token_array_at(const TokenArray ta, const size_t i) {
  return ta->head[i];
}

void token_array_destroy(TokenArray *ta_ptr) {
  if (ta_ptr == NULL || *ta_ptr == NULL) {
    return;
  }
  TokenArray ta = *ta_ptr;
  if (ta->head) {
    // Destroy all tokens before freeing the array
    for (size_t i = 0; i < ta->size; i++) {
      token_destroy(ta->head[i]);
    }
    free(ta->head);
  }
  free(ta);
  *ta_ptr = NULL; // Prevent double-free
}

// ------------------------------------
// LEXER Implementation
// ------------------------------------

const char *const KEYWORD_MAP[] = {
    [TOKEN_LABEL % 100] = "LABEL",   [TOKEN_GOTO % 100] = "GOTO",
    [TOKEN_PRINT % 100] = "PRINT",   [TOKEN_INPUT % 100] = "INPUT",
    [TOKEN_LET % 100] = "LET",       [TOKEN_IF % 100] = "IF",
    [TOKEN_THEN % 100] = "THEN",     [TOKEN_ELSE % 100] = "ELSE",
    [TOKEN_ENDIF % 100] = "ENDIF",   [TOKEN_WHILE % 100] = "WHILE",
    [TOKEN_REPEAT % 100] = "REPEAT", [TOKEN_ENDWHILE % 100] = "ENDWHILE",
};

const char *const OPERATOR_MAP[] = {
    [TOKEN_PLUS % 100] = "+",   [TOKEN_MINUS % 100] = "-",
    [TOKEN_MULT % 100] = "*",   [TOKEN_DIV % 100] = "/",
    [TOKEN_GT % 100] = ">",     [TOKEN_LT % 100] = "<",
    [TOKEN_GTE % 100] = ">=",   [TOKEN_LTE % 100] = "<=",
    [TOKEN_EQ % 100] = "=",     [TOKEN_EQEQ % 100] = "==",
    [TOKEN_NOTEQ % 100] = "!=", [TOKEN_NOT % 100] = "!",
    [TOKEN_AND % 100] = "&&",   [TOKEN_OR % 100] = "||",
};

const char *OPERATOR_CHARS = "+-*/><=!&|";

bool _is_operator_char(const char c) {
  return strchr(OPERATOR_CHARS, c) != NULL;
}

bool _is_alpha_char(const char c) {
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

bool _is_numeric_char(const char c) { return (c >= '0' && c <= '9'); }

bool _is_alpha_numeric_char(const char c) {
  return _is_alpha_char(c) || _is_numeric_char(c);
}

// Custom function to compare string slice with exact token match
bool _string_slice_equals(const char *str_slice, const size_t str_length,
                          const char *token_str) {
  if (token_str == NULL) {
    return false;
  }
  size_t token_len = strlen(token_str);
  if (token_len != str_length) {
    return false;
  }
  return strncmp(str_slice, token_str, str_length) == 0;
}

enum TOKEN _get_token(const char *str_slice, const size_t str_length,
                      const char *const map[], const size_t map_size,
                      const size_t token_offset) {
  for (size_t i = token_offset % 100; i < map_size; i++) {
    if (_string_slice_equals(str_slice, str_length, map[i])) {
      return (enum TOKEN)(i + token_offset);
    }
  }
  return TOKEN_UNKNOWN;
}

size_t strspn_callback(const char *str, bool (*predicate)(char c)) {
  size_t count = 0;
  while (str[count] && predicate(str[count])) {
    count++;
  }
  return count;
}

// Parses tokens from the word, and adds them to the TokenArray
void _lexer_parse_word(const char *const word, TokenArray ta) {
  const size_t word_len = strnlen(word, WORD_BUFFER_SIZE);
  if (word_len == 0) {
    return;
  }
  size_t pos = 0;
  while (true) {
    if (pos == word_len) {
      return;
    }
    // Check for an operation (single or double char)
    if (_is_operator_char(word[pos])) {
      const size_t operator_length = strspn(word + pos, OPERATOR_CHARS);
      const enum TOKEN token =
          _get_token(word + pos, operator_length, OPERATOR_MAP,
                     array_size(OPERATOR_MAP), OPERATOR_START);
      token_array_push_simple(ta, token);
      pos += operator_length;
      continue;
    }
    // Check for a number
    if (_is_numeric_char(word[pos])) {
      const size_t number_length =
          strspn_callback(word + pos, _is_numeric_char);
      token_array_push(ta, TOKEN_NUMBER, word + pos, number_length);
      pos += number_length;
      continue;
    }
    if (_is_alpha_char(word[pos])) {
      const size_t word_length =
          strspn_callback(word + pos, _is_alpha_numeric_char);
      const enum TOKEN token =
          _get_token(word + pos, word_length, KEYWORD_MAP,
                     array_size(KEYWORD_MAP), KEYWORD_START);
      if (token != TOKEN_UNKNOWN) {
        // It's a keyword
        token_array_push_simple(ta, token);
      } else {
        // It's an identifier
        token_array_push(ta, TOKEN_IDENT, word + pos, word_length);
      }
      pos += word_length;
      continue;
    }
    // TODO: REMOVE THIS-- it's just to prevent infinite loops
    pos += 1;
  }
}

TokenArray lexer_parse(FileReader filereader) {
  if (!filereader) {
    return NULL;
  }
  TokenArray ta = token_array_init();
  const char *word;
  while ((word = filereader_read_next_word(filereader)) != NULL) {
    _lexer_parse_word(word, ta);
  }
  return ta;
}