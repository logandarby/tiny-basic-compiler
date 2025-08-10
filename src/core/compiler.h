#pragma once

// -----------------------------------
// COMPILER CONFIG
//
// Defines shared interface to configure the compiler
// -----------------------------------

#include "../common/arg_parse.h"
#include "platform.h"

typedef struct {
  const bool verbose;
  const char *out_file;
  const PlatformInfo target;
  const bool target_is_host; // Flag if the target is equal to the host
} CompilerConfig;

// Initializes a shared compiler config struct from the result of argument
// parsing
CompilerConfig compiler_config_init(ParseResult *result);
