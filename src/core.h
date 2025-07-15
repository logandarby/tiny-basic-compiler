#pragma once

// -------------------------------------
// CORE UTILS
// -------------------------------------

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Like malloc, but panics on failure, and prints an error message
extern void *xmalloc(const size_t size);
