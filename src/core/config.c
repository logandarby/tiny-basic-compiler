#include "config.h"
#include "compiler_compatibility.h"

// DEFAULTS
const char *DEFAULT_OUT_FILE = "a.out";

// MISC CONSTANTS
const char *SEP = "-------------------";

const FlagSpec FLAG_SPEC[] = {
    FLAG('c', "code", "Interpret the input_file as a code string literal"),
// Compiling to another target only available on linux machines
#if defined(__linux__)
    FLAG_WITH_VALUE('t', "target",
                    "Target to assemble to. Target takes the form \"arch-os\". "
                    "Example: x86_64-windows. You must have the requisite gcc "
                    "toolchain installed to use this option."),
    FLAG('l', "list-targets", "List all the supported targets"),
#endif
    FLAG('h', "help", "Show this help message"),
    FLAG('v', "verbose", "Enable verbose output"),
    FLAG_WITH_VALUE('o', "output-file", "The name of the file to output to"),
    FLAG('i', "host-info", "Dump the host info triple"),
    FLAG('a', "emit-asm",
         "Emit the ASM \".s\" file instead of an executable file"),
};

const ArgSpec ARG_SPEC[] = {OPTIONAL_ARG(
    "input_file_or_literal", "The TINY BASIC file to assemble (or code literal "
                             "if compiling with the \"-c\" flag)")};

const ParserSpec PARSER_SPEC =
    PARSER_SPEC("Teeny", "A TINY BASIC compiler", FLAG_SPEC, ARG_SPEC);

// Helpers

bool is_in_array(const int array[], const size_t count, const int elem) {
  for (size_t i = 0; i < count; i++) {
    if (array[i] == elem)
      return true;
  }
  return false;
}

bool is_supported_os(const PlatformInfo *info) {
  return is_in_array((const int *)SUPPORTED_OS, array_size(SUPPORTED_OS),
                     info->os);
}

bool is_supported_arch(const PlatformInfo *info) {
  return is_in_array((const int *)SUPPORTED_ARCH, array_size(SUPPORTED_ARCH),
                     info->arch);
}

void print_supported_platforms(const char *prefix) {
  for (size_t i = 0; i < array_size(SUPPORTED_OS); i++) {
    for (size_t j = 0; j < array_size(SUPPORTED_ARCH); j++) {
      PlatformInfo p = {
          .arch = SUPPORTED_ARCH[j],
          .os = SUPPORTED_OS[i],
          .abi = ABI_UNKNOWN,
      };
      char *triple = platform_info_to_triple(&p);
      if (prefix) {
        printf("%s%s\n", prefix, triple);
      } else {
        printf("%s\n", triple);
      }
      free(triple);
    }
  }
}
