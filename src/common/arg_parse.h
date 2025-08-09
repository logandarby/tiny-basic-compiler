#pragma once

// --------------------------------------
// Argument Parsing
//
// Declarative API for parsing CLI arguments
// --------------------------------------

#include "../core/core.h"

typedef struct {
  const char short_name;     // 'v' for -v
  const char *long_name;     // "verbose" for --verbose
  const char *description;   // Help text
  const bool requires_value; // Whether flag takes an argument
  const bool is_required;    // Whether flag must be present
} FlagSpec;

typedef struct {
  const char *name;        // "input_file"
  const char *description; // Help text
  const bool is_required;  // Whether argument must be present
} ArgSpec;

typedef struct {
  const char *program_name;
  const char *description;
  const FlagSpec *flags;
  const int flag_count;
  const ArgSpec *args;
  const int arg_count;
} ParserSpec;

// Opaque types for the implementation
typedef struct ArgParserHandle ArgParser;
typedef struct ParseResultHandle ParseResult;

// === Main API Functions ===

// Create parser from specification
ArgParser *argparse_create(const ParserSpec *spec);

// Parse command line arguments
ParseResult *argparse_parse(ArgParser *parser, const int argc,
                            const char **argv);

// Query parsed results
bool argparse_has_flag(const ParseResult *result, const char *flag_name);
const char *argparse_get_flag_value(const ParseResult *result,
                                    const char *flag_name);
const char *argparse_get_arg_value(const ParseResult *result,
                                   const char *arg_name);
char **argparse_get_remaining_args(const ParseResult *result, int *count);

// Get error information
bool argparse_is_success(const ParseResult *result);
const char *argparse_get_error(const ParseResult *result);

// Print help message
void argparse_print_help(const ArgParser *parser);

// Cleanup
void argparse_free_parser(ArgParser *parser);
void argparse_free_result(ParseResult *result);

// === Convenience Macros for Specifications ===

// Flag macros
#define FLAG(_short_name, _long_name, _desc)                                   \
  ((FlagSpec){.short_name = _short_name,                                       \
              .long_name = _long_name,                                         \
              .description = _desc,                                            \
              .requires_value = false,                                         \
              .is_required = false})

#define FLAG_WITH_VALUE(_short_name, _long_name, _desc)                        \
  (FlagSpec) {                                                                 \
    .short_name = _short_name, .long_name = _long_name, .description = _desc,  \
    .requires_value = true, .is_required = false                               \
  }

#define REQUIRED_FLAG_WITH_VALUE(_short_name, _long_name, _desc)               \
  (FlagSpec) {                                                                 \
    .short_name = _short_name, .long_name = _long_name, .description = _desc,  \
    .requires_value = true, .is_required = true                                \
  }

// Argument macros
#define REQUIRED_ARG(_name, _desc)                                             \
  (ArgSpec) { .name = _name, .description = _desc, .is_required = true }

#define OPTIONAL_ARG(_name, _desc)                                             \
  (ArgSpec) { .name = _name, .description = _desc, .is_required = false }

// Parser specification macro with automatic count calculation
#define PARSER_SPEC(prog_name, prog_desc, flags_array, args_array)             \
  (ParserSpec) {                                                               \
    .program_name = prog_name, .description = prog_desc, .flags = flags_array, \
    .flag_count = array_size(flags_array), .args = args_array,                 \
    .arg_count = array_size(args_array)                                        \
  }
