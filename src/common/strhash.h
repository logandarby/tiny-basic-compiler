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
#define VERSTABLE 4
#define MLIB_DICT 5

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

#elif STRHASH_IMPL == VERSTABLE

#define NAME strhash_vt
#define KEY_TY const char *
#define VAL_TY void *
#define HASH_FN vt_hash_string
#define CMPR_FN vt_cmpr_string
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#include "verstable.h"
#pragma GCC diagnostic pop

typedef strhash_vt* StrHash;

struct StrHashIter {
    strhash_vt_itr vt_iter;
    strhash_vt *hash;
};

static inline StrHash strhash_init(size_t capacity) {
    StrHash hash = xmalloc(sizeof(strhash_vt));
    strhash_vt_init(hash);
    (void)capacity; // unused in verstable
    return hash;
}

static inline void strhash_free(StrHash hash) {
    if (hash) {
        strhash_vt_cleanup(hash);
        free(hash);
    }
}

static inline bool strhash_put(StrHash *hash, char *key, void *value) {
    strhash_vt_itr itr = strhash_vt_insert(*hash, key, value);
    return !strhash_vt_is_end(itr);
}

static inline void* strhash_get(StrHash hash, char *key) {
    strhash_vt_itr itr = strhash_vt_get(hash, key);
    return strhash_vt_is_end(itr) ? NULL : itr.data->val;
}

static inline size_t strhash_len(StrHash hash) {
    return hash ? strhash_vt_size(hash) : 0;
}

static inline bool strhash_exists(StrHash hash, char *key) {
    strhash_vt_itr itr = strhash_vt_get(hash, key);
    return !strhash_vt_is_end(itr);
}

static inline StrHashIter strhash_iter_start(StrHash hash) {
    StrHashIter iter;
    iter.hash = hash;
    iter.vt_iter = hash ? strhash_vt_first(hash) : (strhash_vt_itr){0};
    return iter;
}

static inline bool strhash_iter_end(StrHashIter iter) {
    return !iter.hash || strhash_vt_is_end(iter.vt_iter);
}

static inline void strhash_iter_next(StrHashIter *iter) {
    if (iter->hash) {
        iter->vt_iter = strhash_vt_next(iter->vt_iter);
    }
}

static inline const char* strhash_iter_key(StrHashIter iter) {
    return iter.vt_iter.data->key;
}

static inline void* strhash_iter_value(StrHashIter iter) {
    return iter.vt_iter.data->val;
}

static inline void strhash_clear(StrHash *hash) {
    if (*hash) {
        strhash_vt_cleanup(*hash);
        strhash_vt_init(*hash);
    }
}

static inline void* strhash_get_or_put(StrHash *hash, char *key, void *default_value) {
    void *val = strhash_get(*hash, key);
    if (!val) {
        strhash_put(hash, key, default_value);
        val = strhash_get(*hash, key);
    }
    return val;
}

// =============================================================================
// MLIB DICT IMPLEMENTATION
// =============================================================================
#elif STRHASH_IMPL == MLIB_DICT

#include "m-dict.h"

// Use basic oplists for pointer types
// Define a dictionary from const char* to void* using basic oplists
M_DICT_DEF2(strhash_mlib, const char *, M_CSTR_OPLIST, void *, M_PTR_OPLIST)

// MLIB generates array types, so we need to wrap it
typedef struct {
    strhash_mlib_t dict;
} mlib_wrapper_t;

typedef mlib_wrapper_t* StrHash;

struct StrHashIter {
    strhash_mlib_it_t it;
    bool is_end;
};

static inline StrHash strhash_init(size_t capacity) {
    StrHash wrapper = xmalloc(sizeof(mlib_wrapper_t));
    strhash_mlib_init(wrapper->dict);
    (void)capacity; // unused in mlib
    return wrapper;
}

static inline void strhash_free(StrHash hash) {
    if (hash) {
        strhash_mlib_clear(hash->dict);
        free(hash);
    }
}

static inline bool strhash_put(StrHash *hash, char *key, void *value) {
    strhash_mlib_set_at((*hash)->dict, key, value);
    return true; // MLIB doesn't return failure for set_at
}

static inline void* strhash_get(StrHash hash, char *key) {
    void **value_ptr = strhash_mlib_get(hash->dict, key);
    return value_ptr ? *value_ptr : NULL;
}

static inline size_t strhash_len(StrHash hash) {
    return hash ? strhash_mlib_size(hash->dict) : 0;
}

static inline bool strhash_exists(StrHash hash, char *key) {
    return hash && strhash_mlib_get(hash->dict, key) != NULL;
}

static inline StrHashIter strhash_iter_start(StrHash hash) {
    StrHashIter iter;
    if (hash) {
        strhash_mlib_it(iter.it, hash->dict);
        iter.is_end = strhash_mlib_end_p(iter.it);
    } else {
        iter.is_end = true;
    }
    return iter;
}

static inline bool strhash_iter_end(StrHashIter iter) {
    return iter.is_end;
}

static inline void strhash_iter_next(StrHashIter *iter) {
    if (!iter->is_end) {
        strhash_mlib_next(iter->it);
        iter->is_end = strhash_mlib_end_p(iter->it);
    }
}

static inline const char* strhash_iter_key(StrHashIter iter) {
    const strhash_mlib_itref_t *ref = strhash_mlib_cref(iter.it);
    return ref->key;
}

static inline void* strhash_iter_value(StrHashIter iter) {
    const strhash_mlib_itref_t *ref = strhash_mlib_cref(iter.it);
    return ref->value;
}

static inline void strhash_clear(StrHash *hash) {
    if (*hash) {
        strhash_mlib_clear((*hash)->dict);
        strhash_mlib_init((*hash)->dict);
    }
}

static inline void* strhash_get_or_put(StrHash *hash, char *key, void *default_value) {
    void *val = strhash_get(*hash, key);
    if (!val) {
        strhash_put(hash, key, default_value);
        val = strhash_get(*hash, key);
    }
    return val;
}

#else
// ============================
// UNSUPPORTED IMPLEMENTATION
// ============================
#error "Unsupported STRHASH_IMPL. Use KHASH, STB_DS, VERSTABLE, or MLIB_DICT"
#endif
