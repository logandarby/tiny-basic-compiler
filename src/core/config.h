#pragma once

// ---------------------------------------------------
// CONFIG
//
// Constants to configure the behaviour of the compiler
// ---------------------------------------------------

#include "arg_parse.h"
#include "platform.h"

// SUPPORT
static const OS SUPPORTED_OS[] = {OS_WINDOWS, OS_LINUX};
static const ARCH SUPPORTED_ARCH[] = {ARCH_X86_64};

bool is_supported_os(const PlatformInfo *info);
bool is_supported_arch(const PlatformInfo *info);
void print_supported_platforms(const char *prefix);

// DEFAULTS
extern const char *DEFAULT_OUT_FILE;

// MISC CONSTANTS
extern const char *SEP;

// COMPILER ARGUMENTS
extern const FlagSpec FLAG_SPEC[];
extern const ArgSpec ARG_SPEC[];
extern const ParserSpec PARSER_SPEC;
