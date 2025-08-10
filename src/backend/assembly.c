#include "assembly.h"

bool assembler_init(AssemblerInfo *cmd, const CompilerConfig *config) {
  const PlatformInfo *target = &config->target;
  if (config->target_is_host) {
    *cmd = (AssemblerInfo){
        .gcc_command = "gcc",
        .assembler_flags = "",
        .linker_flags = "",
        .output_ext = "",
    };
    return true;
  }
  if (target->os == OS_LINUX && target->arch == ARCH_X86_64) {
    *cmd = (AssemblerInfo){.output_ext = "",
                           .linker_flags = "-m64",
                           .assembler_flags = "-m64",
                           .gcc_command = "x86_64-linux-gnu-gcc"};
    return true;
  } else if (target->os == OS_WINDOWS && target->arch == ARCH_X86_64) {
    *cmd = (AssemblerInfo){
        .gcc_command = "x86_64-w64-mingw32-gcc",
        .output_ext = ".exe",
        .assembler_flags = "",
        .linker_flags = "",
    };
  }
  *cmd = (AssemblerInfo){0};
  return false;
}

bool assembler_invoke(AssemblerInfo *cmd, const char *asm_file,
                      const char *output_file) {
  char command[PATH_MAX * 2];
  snprintf(command, sizeof(command), "%s %s %s -x assembler \"%s\" -o \"%s\"",
           cmd->gcc_command, cmd->assembler_flags, cmd->linker_flags, asm_file,
           output_file);
  return system(command) == EXIT_SUCCESS;
}

bool assembler_is_available(AssemblerInfo *cmd) {
  if (!cmd || !cmd->gcc_command)
    return false;
  char command[100];
  snprintf(command, sizeof(command), "%s --version > /dev/null 2>&1",
           cmd->gcc_command);
  return system(command) == EXIT_SUCCESS;
}

void assembler_print_help(AssemblerInfo *cmd) {
  printf("The assembler %s%s%s is not available on your system. Please install "
         "it.",
         KCYN, cmd->gcc_command, KNRM);
}
