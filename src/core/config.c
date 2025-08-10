#include "config.h"

// DEFAULTS
const char *DEFAULT_OUT_FILE = "out";

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
#endif
    FLAG('h', "help", "Show this help message"),
    FLAG('v', "verbose", "Enable verbose output"),
    FLAG_WITH_VALUE('o', "output-file", "The name of the file to output to"),
    FLAG('i', "host-info", "Dump the host info triple"),
};

const ArgSpec ARG_SPEC[] = {OPTIONAL_ARG(
    "input_file_or_literal", "The TINY BASIC file to assemble (or code literal "
                             "if compiling with the \"-c\" flag)")};

const ParserSpec PARSER_SPEC =
    PARSER_SPEC("Teeny", "A TINY BASIC compiler", FLAG_SPEC, ARG_SPEC);
