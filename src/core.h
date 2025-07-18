#pragma once

// -------------------------------------
// CORE UTILS & LIBRARIES
// -------------------------------------

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dz_debug.h"

// =================
// Common Macros
// =================

#define UNUSED(x) (void)(x)
#define array_size(x) (sizeof(x) / sizeof(x[0]))

// =================
// X Allocation Utils (Panic on Failure)
// =================

// Like malloc, but panics on failure, and prints an error message
extern void *xmalloc(const size_t size);
// Like calloc, but panics on failure and prints an error message
extern void *xcalloc(const size_t n, const size_t size);
// Like realloc, but panics on failure and prints an error message
extern void *xrealloc(void *old_ptr, const size_t new_size);
