#define STB_DS_IMPLEMENTATION
#include "core.h"
#define STBDS_REALLOC(context, ptr, size) xrealloc(ptr, size)
#define STBDS_FREE(c, p) free(p)
#include "stb_ds.h"
