#include "token.h"
#include "../../common/arena.h"
#include "../../common/string_util.h"

// Token Array Definitions
const uint32_t INIT_CAPACITY = 512;
const unsigned int CAPACITY_MULTIPLIER = 2;

struct TokenArrayHandle {
  Token *head;
  uint32_t size;     // # of elements stored
  uint32_t capacity; // Total Capacity
  Arena arena;       // Arena allocator for strings
};

// ------------------------------------
// Tokens Implementation
// ------------------------------------

Token token_create(TokenArray ta, enum TOKEN type, const char *text,
                   uint32_t length, const FileLocation location) {
  Token t = {.type = type, .text = NULL, .file_pos = location};
  if (text) {
    t.text = arena_allocate_string(&ta->arena, text, text + length);
  }
  return t;
}

Token token_create_simple(enum TOKEN type, const FileLocation location) {
  Token t = {.type = type, .text = NULL, .file_pos = location};
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

FileLocation token_get_file_pos(const Token token) { return token.file_pos; }

// Destroys any allocated data associated with the Token
void token_destroy(Token token) { UNUSED(token); }

// ------------------------------------
// Token Array Implementation
// ------------------------------------

void _resize_token_array(TokenArray ta, const uint32_t new_size) {
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

void token_array_push_simple(TokenArray ta, enum TOKEN token_type,
                             const FileLocation location) {
  if (ta->size == ta->capacity) {
    _resize_token_array(ta, ta->capacity * CAPACITY_MULTIPLIER);
  }
  ta->head[ta->size] = token_create_simple(token_type, location);
  ta->size++;
}

void token_array_push(TokenArray ta, enum TOKEN token_type, const char *text,
                      uint32_t length, const FileLocation location) {
  if (ta->size == ta->capacity) {
    _resize_token_array(ta, ta->capacity * CAPACITY_MULTIPLIER);
  }
  ta->head[ta->size] = token_create(ta, token_type, text, length, location);
  ta->size++;
}

void token_array_clean_and_push_string(TokenArray ta, const char *text,
                                       const uint32_t length,
                                       const FileLocation location) {

  // Push the string
  token_array_push(ta, TOKEN_STRING, text, length, location);
  // Clean the string -- match for pattern {escape_character}{delmiter}
  Token *current_token = &ta->head[ta->size - 1];
  char *token_text = current_token->text;
  string_clean_escape_sequences(token_text, NULL);
}

uint32_t token_array_length(const TokenArray ta) { return ta->size; }

uint32_t token_array_capacity(const TokenArray ta) { return ta->capacity; }

bool token_array_is_empty(const TokenArray ta) { return ta->size == 0; }

Token token_array_at(const TokenArray ta, const uint32_t i) {
  return ta->head[i];
}

void token_array_destroy(TokenArray *ta_ptr) {
  if (ta_ptr == NULL || *ta_ptr == NULL) {
    return;
  }
  TokenArray ta = *ta_ptr;
  if (ta->head) {
    // Destroy all tokens before freeing the array
    arena_destroy(&ta->arena);
    free(ta->head);
  }
  free(ta);
  *ta_ptr = NULL; // Prevent double-free
}

// Long terrible function, but it does the job since we have exhaustive enums
// turned on in the compiler warnings
const char *token_type_to_string(enum TOKEN type) {
  switch (type) {
  case TOKEN_UNKNOWN:
    return "UNKNOWN";
  case TOKEN_PLUS:
    return "PLUS";
  case TOKEN_MINUS:
    return "MINUS";
  case TOKEN_MULT:
    return "MULT";
  case TOKEN_DIV:
    return "DIV";
  case TOKEN_GT:
    return "GT";
  case TOKEN_LT:
    return "LT";
  case TOKEN_GTE:
    return "GTE";
  case TOKEN_LTE:
    return "LTE";
  case TOKEN_EQ:
    return "EQ";
  case TOKEN_NOTEQ:
    return "NOTEQ";
  case TOKEN_EQEQ:
    return "EQEQ";
  case TOKEN_NOT:
    return "NOT";
  case TOKEN_AND:
    return "AND";
  case TOKEN_OR:
    return "OR";
  case TOKEN_STRING:
    return "STRING";
  case TOKEN_NUMBER:
    return "NUMBER";
  case TOKEN_IDENT:
    return "IDENT";
  case TOKEN_LABEL:
    return "LABEL";
  case TOKEN_PRINT:
    return "PRINT";
  case TOKEN_INPUT:
    return "INPUT";
  case TOKEN_LET:
    return "LET";
  case TOKEN_IF:
    return "IF";
  case TOKEN_GOTO:
    return "GOTO";
  case TOKEN_THEN:
    return "THEN";
  case TOKEN_ELSE:
    return "ELSE";
  case TOKEN_ENDIF:
    return "ENDIF";
  case TOKEN_WHILE:
    return "WHILE";
  case TOKEN_REPEAT:
    return "REPEAT";
  case TOKEN_ENDWHILE:
    return "ENDWHILE";
  }
  return "";
}
