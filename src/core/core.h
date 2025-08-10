#pragma once

// -------------------------------------
// CORE UTILS & LIBRARIES
// -------------------------------------

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../debug/dz_debug.h"

#if defined(_WIN32) || defined(_WIN64)
#include <io.h>
#include <windows.h>
#ifndef PATH_MAX
#define PATH_MAX MAX_PATH
#endif // !PATH_MAX
#else
#include <linux/limits.h>
#endif

// =================
// X Allocation Utils (Panic on Failure)
// =================

// Like malloc, but panics on failure, and prints an error message
void *xmalloc(const uint32_t size);
// Like calloc, but panics on failure and prints an error message
void *xcalloc(const uint32_t n, const uint32_t size);
// Like realloc, but panics on failure and prints an error message
void *xrealloc(void *old_ptr, const uint32_t new_size);
