#pragma once

// -----------------------------------
// COMPILER CORE LOGIC
//
// Defines
// -----------------------------------

#include "../common/arg_parse.h"
#include "platform.h"

typedef enum {
  EMIT_EXECUTABLE, // Default. Checks that an assembler is available
  EMIT_X86_ASSEMBLY,
} EMIT_FORMAT;

typedef struct {
  char *filename_or_code_literal;
  const bool
      is_code_literal; // If the given arg is a filename, or a code literal
  const bool verbose;  // Verbose option
  const EMIT_FORMAT emit_format; // The format which the compiler should emit
                                 // (just the assembly, or an executable)
  char *out_file;
  const PlatformInfo target;
  char *triple; // triple input by the user/host triple if none was provided
  const bool target_is_host; // Flag if the target is equal to the host
} CompilerConfig;

// Initializes a shared compiler config struct from the result of argument
// parsing
// Must call compiler_config_free after
CompilerConfig compiler_config_init(ParseResult *result);

void compiler_config_free(CompilerConfig *config);

// Executes a compiler with a config
// Returns whether or not it was successful
bool compiler_execute(const CompilerConfig *config);

void compiler_error(const char *restrict msg, ...) FORMAT_PRINTF(1, 2);
