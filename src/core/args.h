#pragma once

// ------------------------------------
// ARGUMENT PARSING
// ------------------------------------

#include <stdbool.h>

// Return value after parsing the program arguments
// See that we may either return a filename which must be read, or literal code
// which can be read as such
typedef struct Args *Args;
// Gets the filename/code literal from the arguments
// If the argument does not exist, or the incorrect arguments are supplied, it
// gives an error, prints the usage, and exits.
// Must call args_free after using
Args parse_args(const int argc, const char **argv);
void args_free(Args *args);

// Depending on the arg type, can either get the filename, or the code literal.
// If you try and get the wrong kind of argument, the code will panic.
const char *args_get_filename(Args args);
// Depending on the arg type, can either get the filename, or the code literal.
// If you try and get the wrong kind of argument, the code will panic.
const char *args_get_code_literal(Args args);

bool args_is_filename(Args args);
bool args_is_code_literal(Args args);

void args_print_usage(void);
