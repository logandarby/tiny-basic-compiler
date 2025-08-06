#include "string_util.h"

// Takes an escaped character (for example, \n), and returns the desired literal
// character associated with it (in this case, a newline)
static char _lookup_escape_replacement(char escaped_char,
                                       const EscapeConfig *config,
                                       bool *found) {
  *found = false;
  for (uint32_t i = 0; i < config->count; i++) {
    if (config->mappings[i].escape_char == escaped_char) {
      *found = true;
      return config->mappings[i].replacement;
    }
  }
  return '\0';
}

bool string_clean_escape_sequences(char *input, const EscapeConfig *config) {
  if (input == NULL) {
    return false;
  }
  if (config == NULL) {
    config = &DEFAULT_ESCAPE_CONFIG;
  }
  const uint32_t input_len = strlen(input);
  // Empty string, nothing to do.
  if (input_len == 0) {
    return true;
  }
  uint32_t read_idx = 0;  // Where to read next char
  uint32_t write_idx = 0; // Where to write next char
  while (read_idx < input_len) {
    if (input[read_idx] == ESCAPE_PREFIX && read_idx + 1 < input_len) {
      // Process potential escape sequence
      char escape_char = input[read_idx + 1];
      bool mapping_found;
      char replacement =
          _lookup_escape_replacement(escape_char, config, &mapping_found);

      if (mapping_found) {
        // Valid escape sequence - replace it
        input[write_idx++] = replacement;
        read_idx += ESCAPE_SEQUENCE_LENGTH;
      } else if (config->preserve_unknown) {
        // Unknown escape sequence - preserve as-is
        input[write_idx++] = ESCAPE_PREFIX;
        input[write_idx++] = escape_char;
        read_idx += ESCAPE_SEQUENCE_LENGTH;
      } else {
        // Unknown escape sequence - remove backslash
        input[write_idx++] = escape_char;
        read_idx += ESCAPE_SEQUENCE_LENGTH;
      }
    } else {
      // Regular character - copy if needed
      if (write_idx != read_idx) {
        input[write_idx] = input[read_idx];
      }
      write_idx++;
      read_idx++;
    }
  }

  // Null terminate the string
  input[write_idx] = '\0';
  return true;
}

// Custom function to compare string slice with exact token match
bool string_slice_equals(const char *str_slice, const uint32_t str_length,
                         const char *token_str) {
  if (token_str == NULL) {
    return false;
  }
  uint32_t token_len = strlen(token_str);
  if (token_len != str_length) {
    return false;
  }
  return strncmp(str_slice, token_str, str_length) == 0;
}

uint32_t strspn_callback(const char *str, bool (*predicate)(char c)) {
  uint32_t count = 0;
  while (str[count] && predicate(str[count])) {
    count++;
  }
  return count;
}

void strip_newline(const char *restrict str, char *restrict buffer,
                   uint32_t buffer_size) {
  DZ_ASSERT(str && buffer && buffer_size > 0);
  uint32_t write_pos = 0;
  for (uint32_t read_pos = 0;
       str[read_pos] != '\0' && write_pos < buffer_size - 1; read_pos++) {
    if (str[read_pos] != '\n' && str[read_pos] != '\r') {
      buffer[write_pos++] = str[read_pos];
    }
  }
  buffer[write_pos] = '\0';
}

void strip_trailing_newlines(char *str, const uint32_t n) {
  if (!str)
    return; // Handle null pointer
  uint32_t len = strnlen(str, n);
  if (len < n) {
  }
  if (len == 0)
    return; // Handle empty string
  // Work backwards from the end
  while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
    str[--len] = '\0';
  }
}
