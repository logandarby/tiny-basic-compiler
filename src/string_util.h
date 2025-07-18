#pragma once

// ----------------------------------
// String Util
//
// Contains utils to clean strings from escaped characters
// ----------------------------------

#include "core.h"

static const char ESCAPE_PREFIX = '\\';
static const size_t ESCAPE_SEQUENCE_LENGTH = 2;

// Structure to define an escape mapping
typedef struct {
  char escape_char;
  char replacement;
} EscapeMappingNode;

// Srtucture to define what escaped characters map to which real characters
typedef struct {
  const EscapeMappingNode *mappings;
  size_t count;
  bool preserve_unknown;
} EscapeConfig;

static const EscapeMappingNode DEFAULT_ESCAPE_MAPPINGS[] = {
    {'"', '"'},   // \" -> "
    {'\'', '\''}, // \' -> '
    {'\\', '\\'}, // \\ -> '\'
    {'n', '\n'},  // \n -> newline
    {'t', '\t'},  // \t -> tab
    {'r', '\r'},  // \r -> carriage return
    {'b', '\b'},  // \b -> backspace
    {'f', '\f'},  // \f -> form feed
    {'v', '\v'},  // \v -> vertical tab
    {'0', '\0'},  // \0 -> null character
};

static const EscapeConfig DEFAULT_ESCAPE_CONFIG = {
    .mappings = DEFAULT_ESCAPE_MAPPINGS,
    .count = array_size(DEFAULT_ESCAPE_MAPPINGS),
    .preserve_unknown = true,
};

// Cleans a string in place. Cleans a string and replaces all possible escape
// sequenceis based on the EscapeConfig. Returns whether it succeeded or not.
// The EscapeConfig is OPTIONAL, and is replaced with this
// DEFAULT_ESCAPE_MAPPINGS
bool string_clean_escape_sequences(char *input, const EscapeConfig *config);
