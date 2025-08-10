#pragma once

// --------------------------------
// ASSEMBLY
//
// Defines utils for asssmbling file
// in a platform agnostic way
// --------------------------------

#include "../core/core.h"
#include "compiler.h"

typedef struct {
  const char *gcc_command;
  const char *assembler_flags;
  const char *linker_flags;
  const char *output_ext;
} AssemblerInfo;

// Initializes an asm command inside cmd based off a compiler config.
bool assembler_init(AssemblerInfo *cmd, const CompilerConfig *config);

// Invokes the compiler command on the system
// Compiles asm_file into the output_file
// Returns if it succeeds
bool assembler_invoke(AssemblerInfo *cmd, const char *asm_file,
                      const char *output_file);

// Checks if the command is installed on the host platform
bool assembler_is_available(AssemblerInfo *cmd);

// Prints a help message based off the command
void assembler_print_help(AssemblerInfo *cmd);
