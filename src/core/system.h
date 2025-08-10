#pragma once

// -----------------------------------
// SYSTEM UTILS
//
// Platform specific functionality wrapped in system
// agnostic functions
// -----------------------------------

#include "core.h"

// Creates a named temporary file. The file pointer is returned, and the name
// Is put inside filepath. You should allocate filepath with MAX_PATH
// characters.
FILE *create_named_tmpfile(char *filepath, size_t filepath_size);
