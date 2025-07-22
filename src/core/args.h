#pragma once

// ------------------------------------
// ARGUMENT PARSING
// ------------------------------------

// Return value after parsing the program arguments
struct Args {
  const char *filename;
};

// Gets the filename from the arguments
// If the argument does not exist, or the incorrect arguments are supplied, it
// gives an error, prints the usage, and exits.
struct Args parse_args(const int argc, const char **argv);
