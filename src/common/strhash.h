// -----------------------------------
// STRING HASH
//
// A flexible string hash table facade that can swap implementations at compile
// time This is for testing performance for critical data structures Supports:
// KHASH, STB_DS, and others Usage: #define STRHASH_IMPL KHASH before including
// this header
// -----------------------------------

#pragma once

#include "../core/core.h"
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// Implementation selection
#ifndef STRHASH_IMPL
#define STRHASH_IMPL STB_DS
#endif

#define KHASH 1
#define STB_DS 2
#define CC_TABLE 3
#define VERSTABLE 4

// ============================
// FORWARD DECLARATIONS - these functions are implemented for each backend
// ============================

// Iterator structure (implementation-specific)
typedef struct StrHashIter StrHashIter;

// StrHash typedef and all functions are defined in implementation blocks below

// ============================
// KHASH IMPLEMENTATION
// ============================
#if STRHASH_IMPL == KHASH

#include "khash.h"
KHASH_MAP_INIT_STR(str, void *)

typedef khash_t(str) * StrHash;

struct StrHashIter {
  khash_t(str) * hash;
  khint_t index;
};

static inline StrHash strhash_init(size_t capacity) {
  (void)capacity; // unused in khash
  return kh_init(str);
}

static inline void strhash_free(StrHash hash) {
  if (hash)
    kh_destroy(str, hash);
}

static inline bool strhash_put(StrHash *hash, char *key, void *value) {
  int ret;
  khint_t k = kh_put(str, *hash, key, &ret);
  if (ret >= 0) {
    kh_value(*hash, k) = value;
    return true;
  }
  return false;
}

static inline void *strhash_get(StrHash hash, char *key) {
  khint_t k = kh_get(str, hash, key);
  return (k != kh_end(hash)) ? kh_value(hash, k) : NULL;
}

static inline size_t strhash_len(StrHash hash) {
  return hash ? kh_size(hash) : 0;
}

static inline bool strhash_exists(StrHash hash, char *key) {
  khint_t k = kh_get(str, hash, key);
  return (k != kh_end(hash));
}

static inline StrHashIter strhash_iter_start(StrHash hash) {
  StrHashIter iter = {hash, 0};
  if (hash) {
    while (iter.index < kh_end(hash) && !kh_exist(hash, iter.index))
      iter.index++;
  }
  return iter;
}

static inline bool strhash_iter_end(StrHashIter iter) {
  return !iter.hash || iter.index >= kh_end(iter.hash);
}

static inline void strhash_iter_next(StrHashIter *iter) {
  if (iter->hash) {
    iter->index++;
    while (iter->index < kh_end(iter->hash) &&
           !kh_exist(iter->hash, iter->index))
      iter->index++;
  }
}

static inline const char *strhash_iter_key(StrHashIter iter) {
  return kh_key(iter.hash, iter.index);
}

static inline void *strhash_iter_value(StrHashIter iter) {
  return kh_value(iter.hash, iter.index);
}

static inline void strhash_clear(StrHash *hash) {
  strhash_free(*hash);
  *hash = strhash_init(0);
}

static inline void *strhash_get_or_put(StrHash *hash, char *key,
                                       void *default_value) {
  void *val = strhash_get(*hash, key);
  if (!val) {
    strhash_put(hash, key, default_value);
    val = strhash_get(*hash, key);
  }
  return val;
}

// ============================
// STB_DS IMPLEMENTATION
// ============================
#elif STRHASH_IMPL == STB_DS
#include "stb_ds.h"

typedef struct {
  char *key;
  void *value;
} strhash_entry_t;

typedef strhash_entry_t *StrHash;

struct StrHashIter {
  strhash_entry_t *hash;
  size_t index;
  size_t length;
};

static inline StrHash strhash_init(size_t capacity) {
  (void)capacity; // unused
  return NULL;
}

static inline void strhash_free(StrHash hash) {
  if (hash)
    shfree(hash);
}

static inline bool strhash_put(StrHash *hash, char *key, void *value) {
  shput(*hash, key, value);
  return true;
}

static inline void *strhash_get(StrHash hash, char *key) {
  return (hash && shgeti(hash, key) >= 0) ? shget(hash, key) : NULL;
}

static inline size_t strhash_len(StrHash hash) {
  return hash ? shlen(hash) : 0;
}

static inline bool strhash_exists(StrHash hash, char *key) {
  return (hash && shgeti(hash, key) >= 0);
}

static inline StrHashIter strhash_iter_start(StrHash hash) {
  StrHashIter iter = {hash, 0, hash ? shlen(hash) : 0};
  return iter;
}

static inline bool strhash_iter_end(StrHashIter iter) {
  return iter.index >= iter.length;
}

static inline void strhash_iter_next(StrHashIter *iter) { iter->index++; }

static inline const char *strhash_iter_key(StrHashIter iter) {
  return iter.hash[iter.index].key;
}

static inline void *strhash_iter_value(StrHashIter iter) {
  return iter.hash[iter.index].value;
}

static inline void strhash_clear(StrHash *hash) {
  strhash_free(*hash);
  *hash = strhash_init(0);
}

static inline void *strhash_get_or_put(StrHash *hash, char *key,
                                       void *default_value) {
  void *val = strhash_get(*hash, key);
  if (!val) {
    strhash_put(hash, key, default_value);
    val = strhash_get(*hash, key);
  }
  return val;
}

// ============================
// UNSUPPORTED IMPLEMENTATION
// ============================
#else
#error "Unsupported STRHASH_IMPL. Use KHASH or STB_DS"
#endif
