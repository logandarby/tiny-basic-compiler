#include "lexer.h"
#include "core.h"
#include "file.h"
#include "string_util.h"
#include <assert.h>
#include <string.h>

const size_t INIT_CAPACITY = 512;
const unsigned int CAPACITY_MULTIPLIER = 2;

// ------------------------------------
// Tokens Implementation
// ------------------------------------

Token token_create(enum TOKEN type, const char *text, size_t length) {
  Token t = {.type = type, .text = NULL};
  if (text) {
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

void token_array_clean_and_push_string(TokenArray ta, const char *text,
                                       const size_t length) {

  // Push the string
  token_array_push(ta, TOKEN_STRING, text, length);
  // Clean the string -- match for pattern {escape_character}{delmiter}
  Token *current_token = &ta->head[ta->size - 1];
  char *token_text = current_token->text;
  string_clean_escape_sequences(token_text, NULL);
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
const char *STRING_DELIMS = "'\"";
const char *WHITESPACE_CHARS = " \t\n\r\f\v";
const char ESCAPE_CHAR = '\\';

bool _is_whitespace_char(const char c) {
  return strchr(WHITESPACE_CHARS, c) != NULL;
}

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

bool _is_string_delim(const char c) { return strchr(STRING_DELIMS, c) != NULL; }

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

enum LEXER_STATE { LEXER_STATE_NORMAL, LEXER_STATE_PARSING_STRING };

typedef struct {
  enum LEXER_STATE state;
  char current_string_delim; // Stores the string delimiter
} LexerState;

// Parses tokens from the line, and adds them to the TokenArray
void _lexer_parse_line(const char *const line, const size_t max_line_length,
                       TokenArray ta) {
  // FSM State
  LexerState state = {
      .current_string_delim = '\0',
      .state = LEXER_STATE_NORMAL,
  };
  const size_t line_length = strnlen(line, max_line_length);
  if (line_length == 0) {
    return;
  }
  size_t pos = 0;
  // FSM For line
  while (pos < line_length) {
    // If in string state, look for next delimiter
    if (state.state == LEXER_STATE_PARSING_STRING) {
      // Skip first delimiter
      pos++;
      const char delimiter_check[2] = {state.current_string_delim, '\0'};
      size_t string_start = pos;
      size_t current_pos = pos;
      while (current_pos < line_length) {
        current_pos += strcspn(line + current_pos, delimiter_check);
        if (current_pos == line_length) {
          // Unterminated string
          break;
        }
        // Check if the delimiter is escaped
        if (current_pos > string_start &&
            line[current_pos - 1] == ESCAPE_CHAR &&
            line[current_pos] == state.current_string_delim) {
          current_pos++;
          continue;
        }
        // Found closing delimiter
        break;
      }
      const size_t string_length = current_pos - string_start;
      // If it reached the end of line, it didn't find the delimiter, so the
      // token is unknown
      if (pos + string_length == line_length) {
        token_array_push_simple(ta, TOKEN_UNKNOWN);
      } else {
        // Clean string of any escaped characters, and push the string
        token_array_clean_and_push_string(ta, line + pos, string_length);
        // advance past the last string delimiter
        pos++;
      }
      // Return state back
      state.state = LEXER_STATE_NORMAL;
      state.current_string_delim = '\0';
      pos += string_length;
      continue;
    }
    // Eat Whitespace
    if (_is_whitespace_char(line[pos])) {
      const size_t whitespace_len = strspn(line + pos, WHITESPACE_CHARS);
      pos += whitespace_len;
      continue;
    }
    // Check for start of a string, and set state to a string
    if (_is_string_delim(line[pos])) {
      state.current_string_delim = line[pos];
      state.state = LEXER_STATE_PARSING_STRING;
      continue; // Exit word parsing, let main loop handle string parsing
    }
    // Check for an operation (single or double char)
    if (_is_operator_char(line[pos])) {
      const size_t operator_length = strspn(line + pos, OPERATOR_CHARS);
      const enum TOKEN token =
          _get_token(line + pos, operator_length, OPERATOR_MAP,
                     array_size(OPERATOR_MAP), OPERATOR_START);
      token_array_push_simple(ta, token);
      pos += operator_length;
      continue;
    }
    // Check for a number
    if (_is_numeric_char(line[pos])) {
      const size_t number_length =
          strspn_callback(line + pos, _is_numeric_char);
      token_array_push(ta, TOKEN_NUMBER, line + pos, number_length);
      pos += number_length;
      continue;
    }
    // Check for an identifier or keyword
    if (_is_alpha_char(line[pos])) {
      const size_t word_length =
          strspn_callback(line + pos, _is_alpha_numeric_char);
      const enum TOKEN token =
          _get_token(line + pos, word_length, KEYWORD_MAP,
                     array_size(KEYWORD_MAP), KEYWORD_START);
      if (token != TOKEN_UNKNOWN) {
        // It's a keyword
        token_array_push_simple(ta, token);
      } else {
        // It's an identifier
        token_array_push(ta, TOKEN_IDENT, line + pos, word_length);
      }
      pos += word_length;
      continue;
    }
    token_array_push_simple(ta, TOKEN_UNKNOWN);
    pos += 1;
  }
}

TokenArray lexer_parse(FileReader filereader) {
  if (!filereader) {
    return NULL;
  }
  TokenArray ta = token_array_init();

  while (true) {
    // Use word-based parsing for normal tokens
    const char *line = filereader_read_next_line(filereader);
    if (!line) {
      break; // EOF reached
    }
    _lexer_parse_line(line, filereader_get_linebuffer_length(filereader), ta);
  }
  return ta;
}