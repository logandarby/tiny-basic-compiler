#include "lexer.h"
#include "../../common/error_reporter.h"
#include "../../common/file_reader.h"
#include "../../common/string_util.h"
#include "dz_debug.h"
#include <assert.h>
#include <string.h>

// ----------------------------------
// Constants, Definitions, and Macros
// ----------------------------------

// Lexer constants & macros

// Converts token enum values to array indices by extracting the offset within
// each category. E.g., TOKEN_LABEL (300) -> 0, TOKEN_PRINT (301) -> 1, etc.
#define _index_of(token) ((token) % TOKEN_CATEGORY_SPACING)

const char *const KEYWORD_MAP[] = {
    [_index_of(TOKEN_LABEL)] = "LABEL",
    [_index_of(TOKEN_GOTO)] = "GOTO",
    [_index_of(TOKEN_PRINT)] = "PRINT",
    [_index_of(TOKEN_INPUT)] = "INPUT",
    [_index_of(TOKEN_LET)] = "LET",
    [_index_of(TOKEN_IF)] = "IF",
    [_index_of(TOKEN_THEN)] = "THEN",
    [_index_of(TOKEN_ELSE)] = "ELSE",
    [_index_of(TOKEN_ENDIF)] = "ENDIF",
    [_index_of(TOKEN_WHILE)] = "WHILE",
    [_index_of(TOKEN_REPEAT)] = "REPEAT",
    [_index_of(TOKEN_ENDWHILE)] = "ENDWHILE",
};

const char *const OPERATOR_MAP[] = {
    [_index_of(TOKEN_PLUS)] = "+",   [_index_of(TOKEN_MINUS)] = "-",
    [_index_of(TOKEN_MULT)] = "*",   [_index_of(TOKEN_DIV)] = "/",
    [_index_of(TOKEN_GT)] = ">",     [_index_of(TOKEN_LT)] = "<",
    [_index_of(TOKEN_GTE)] = ">=",   [_index_of(TOKEN_LTE)] = "<=",
    [_index_of(TOKEN_EQ)] = "=",     [_index_of(TOKEN_EQEQ)] = "==",
    [_index_of(TOKEN_NOTEQ)] = "!=", [_index_of(TOKEN_NOT)] = "!",
    [_index_of(TOKEN_AND)] = "&&",   [_index_of(TOKEN_OR)] = "||",
};

const char *OPERATOR_CHARS = "+-*/><=!&|";
const char *STRING_DELIMS = "'\"";
const char *WHITESPACE_CHARS = " \t\n\r\f\v";
const char ESCAPE_CHAR = '\\';

enum LEXER_STATE { LEXER_STATE_NORMAL, LEXER_STATE_PARSING_STRING };

typedef struct {
  enum LEXER_STATE state;
  char current_string_delim; // Stores the string delimiter
} LexerState;

// ------------------------------------
// LEXER Implementation
// ------------------------------------

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

// Variables must start with a numeric character, but then contain numbers or
// underscores
bool _is_variable_char(const char c) {
  return _is_alpha_char(c) || _is_numeric_char(c) || c == '_';
}

bool _is_string_delim(const char c) { return strchr(STRING_DELIMS, c) != NULL; }

enum TOKEN get_token_from_string(const char *str_slice,
                                 const uint32_t str_length,
                                 const char *const map[],
                                 const uint32_t map_size,
                                 const uint32_t token_offset) {
  for (uint32_t i = _index_of(token_offset); i < map_size; i++) {
    if (string_slice_equals(str_slice, str_length, map[i])) {
      return (enum TOKEN)(i + token_offset);
    }
  }
  return TOKEN_UNKNOWN;
}

// Parses tokens from the line, and adds them to the TokenArray
void _lexer_parse_line(const char *const line, const FileReader fr,
                       TokenArray ta) {
  const uint32_t max_line_length = filereader_get_linebuffer_length(fr);
  const uint32_t line_number = filereader_get_current_line_number(fr);
  const char *filename = filereader_get_filename_ref(fr);
  // FSM State
  LexerState state = {
      .current_string_delim = '\0',
      .state = LEXER_STATE_NORMAL,
  };
  const uint32_t line_length = strnlen(line, max_line_length);
  if (line_length == 0) {
    return;
  }
  uint32_t pos = 0;
  // FSM For line
  while (pos < line_length) {
    // If in string state, look for next delimiter
    if (state.state == LEXER_STATE_PARSING_STRING) {
      // Skip first delimiter
      pos++;
      const char delimiter_check[2] = {state.current_string_delim, '\0'};
      uint32_t string_start = pos;
      uint32_t current_pos = pos;
      if (current_pos == line_length) {
        // ERROR: Unterminated empty string at end of line
        er_add_error(ERROR_LEXICAL, filename, line_number, pos,
                     "Unterminated empty string. Remove the dangling delimiter "
                     "(%s%c%s) at the end of the line.",
                     KRED, state.current_string_delim, KNRM);
      }
      while (current_pos < line_length) {
        current_pos += strcspn(line + current_pos, delimiter_check);
        if (current_pos >= line_length) {
          // ERROR: Unterminated string
          char *bad_string = malloc(line_length + 1);
          strip_newline(line + pos, bad_string, line_length + 1);
          er_add_error(ERROR_LEXICAL, filename, line_number, pos,
                       "Unterminated string \"%s\". Make sure to end your "
                       "strings with the delimiter %c",
                       bad_string, state.current_string_delim);
          free(bad_string);
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
      const uint32_t string_length = current_pos - string_start;
      // Get line:col of file (1-indexed)
      const FileLocation file_location = {
          .line = line_number,
          .col = pos + 1,
      };
      // If it reached the end of line, it didn't find the delimiter, so the
      // token is unknown
      if (pos + string_length == line_length) {
        // Error reporting handled earlier
        token_array_push_simple(ta, TOKEN_UNKNOWN, file_location);
      } else {
        // Clean string of any escaped characters, and push the string
        token_array_clean_and_push_string(ta, line + pos, string_length,
                                          file_location);
        // advance past the last string delimiter
        pos++;
      }
      // Return state back
      state.state = LEXER_STATE_NORMAL;
      state.current_string_delim = '\0';
      pos += string_length;
      continue;
    }
    // Get line:col of file (1-indexed)
    const FileLocation file_location = {
        .line = line_number,
        .col = pos + 1,
    };
    // Eat Whitespace
    if (_is_whitespace_char(line[pos])) {
      const uint32_t whitespace_len = strspn(line + pos, WHITESPACE_CHARS);
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
      const uint32_t operator_length = strspn(line + pos, OPERATOR_CHARS);
      const enum TOKEN token =
          get_token_from_string(line + pos, operator_length, OPERATOR_MAP,
                                array_size(OPERATOR_MAP), OPERATOR_START);
      token_array_push_simple(ta, token, file_location);
      pos += operator_length;
      continue;
    }
    // Check for a number
    if (_is_numeric_char(line[pos])) {
      const uint32_t number_length =
          strspn_callback(line + pos, _is_numeric_char);
      token_array_push(ta, TOKEN_NUMBER, line + pos, number_length,
                       file_location);
      pos += number_length;
      continue;
    }
    // Check for an identifier or keyword
    if (_is_alpha_char(line[pos])) {
      const uint32_t word_length =
          strspn_callback(line + pos, _is_variable_char);
      const enum TOKEN token =
          get_token_from_string(line + pos, word_length, KEYWORD_MAP,
                                array_size(KEYWORD_MAP), KEYWORD_START);
      if (token != TOKEN_UNKNOWN) {
        // It's a keyword
        token_array_push_simple(ta, token, file_location);
      } else {
        // It's an identifier
        token_array_push(ta, TOKEN_IDENT, line + pos, word_length,
                         file_location);
      }
      pos += word_length;
      continue;
    }
    // ERROR: Somehow the token is unknown. Report it.
    const char bad_char = line[pos];
    er_add_error(ERROR_LEXICAL, filename, line_number, pos + 1,
                 "Invalid character \"%s%c%s\" (hex code %02X) encountered. "
                 "Please only use basic ASCII characters in your code.",
                 KRED, bad_char, KNRM, (unsigned char)bad_char);
    token_array_push_simple(ta, TOKEN_UNKNOWN, file_location);
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
    _lexer_parse_line(line, filereader, ta);
  }
  return ta;
}
