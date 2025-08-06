#pragma once

/**
 * compiler.h - Compiler compatibility layer
 *
 * Provides portable macros for compiler-specific attributes and builtins.
 * Supports GCC, Clang, and falls back gracefully on other compilers.
 */

#include <stdbool.h>
#include <stddef.h>

/* ========================================================================== */
/*                              COMPILER DETECTION                           */
/* ========================================================================== */

// Define the COMPILER as either of these

#define COMPILER_GCC 1
#define COMPILER_CLANG 2
#define COMPILER_MSVC 3
#define COMPILER_UNKNOWN 4

#if defined(__GNUC__) && !defined(__clang__)
#define COMPILER COMPILER_GCC
#define COMPILER_VERSION                                                       \
  (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#elif defined(__clang__)
#define COMPILER COMPILER_CLANG
#define COMPILER_VERSION                                                       \
  (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
#elif defined(_MSC_VER)
#define COMPILER COMPILER_MSVC
#define COMPILER_VERSION _MSC_VER
#else
#define COMPILER COMPILER_UNKNOWN
#define COMPILER_VERSION 0
#endif

/* Check for GCC-compatible compiler (GCC or Clang) */
#define COMPILER_HAS_GNU_ATTRIBUTES                                            \
  (COMPILER == COMPILER_GCC || COMPILER == COMPILER_CLANG)

/* ========================================================================== */
/*                              FUNCTION ATTRIBUTES                          */
/* ========================================================================== */

#if COMPILER_HAS_GNU_ATTRIBUTES
/* Function never returns (exit, abort, panic, etc.) */
#define NORETURN __attribute__((noreturn))
/* Function has no side effects and return depends only on parameters */
#define CONST __attribute__((const))
/* Function has no side effects but may read global state */
#define PURE __attribute__((pure))
/* Function result should not be ignored */
#define WARN_UNUSED_RESULT __attribute__((warn_unused_result))
/* Function takes printf-style format string (func, format_idx, args_idx) */
#define FORMAT_PRINTF(fmt_idx, args_idx)                                       \
  __attribute__((format(printf, fmt_idx, args_idx)))
/* Function returns fresh memory (malloc-style) */
#define MALLOC_LIKE __attribute__((malloc))
/* Function is deprecated */
#define DEPRECATED __attribute__((deprecated))
/* Function should always be inlined */
#define FORCE_INLINE __attribute__((always_inline)) inline
/* Function should never be inlined */
#define NEVER_INLINE __attribute__((noinline))

#elif COMPILER == COMPILER_MSVC
#define NORETURN __declspec(noreturn)
#define CONST
#define PURE
#define WARN_UNUSED_RESULT
#define FORMAT_PRINTF(fmt_idx, args_idx)
#define MALLOC_LIKE __declspec(restrict)
#define DEPRECATED __declspec(deprecated)
#define FORCE_INLINE __forceinline
#define NEVER_INLINE __declspec(noinline)

#else
/* Fallback for unknown compilers */
#define NORETURN
#define CONST
#define PURE
#define WARN_UNUSED_RESULT
#define FORMAT_PRINTF(fmt_idx, args_idx)
#define MALLOC_LIKE
#define DEPRECATED
#define FORCE_INLINE inline
#define NEVER_INLINE
#endif

/* ========================================================================== */
/*                              VARIABLE ATTRIBUTES                          */
/* ========================================================================== */

#define UNUSED(x) ((void)(x))

#if COMPILER_HAS_GNU_ATTRIBUTES
/* Variable is intentionally unused (silence warnings) */
#define UNUSED_FUNC __attribute__((unused))
/* Pack struct/union (no padding) */
#define PACKED __attribute__((packed))
/* Align variable to N bytes */
#define ALIGNED(n) __attribute__((aligned(n)))
/* Variable may alias other variables (disable strict aliasing) */
#define MAY_ALIAS __attribute__((may_alias))

#elif COMPILER == COMPILER_MSVC
#define UNUSED_FUNC ((void))
#define PACKED
#define ALIGNED(n) __declspec(align(n))
#define MAY_ALIAS

#else
#define UNUSED_FUNC
#define PACKED
#define ALIGNED(n)
#define MAY_ALIAS
#endif

/* ========================================================================== */
/*                              BRANCH PREDICTION                            */
/* ========================================================================== */

#if COMPILER_HAS_GNU_ATTRIBUTES
/* Hint that expression is likely to be true */
#define LIKELY(x) __builtin_expect(!!(x), 1)
/* Hint that expression is likely to be false */
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#else
#define LIKELY(x) (x)
#define UNLIKELY(x) (x)
#endif

/* ========================================================================== */
/*                              UTILITY MACROS                               */
/* ========================================================================== */

/* Compile-time assertion */
#ifdef _Static_assert
#define STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#elif COMPILER_HAS_GNU_ATTRIBUTES
#define STATIC_ASSERT(cond, msg)                                               \
  typedef char static_assert_##__LINE__[(cond) ? 1 : -1] UNUSED
#else
#define STATIC_ASSERT(cond, msg)                                               \
  do {                                                                         \
  } while (0)
#endif

/* Array size that only works on arrays, not pointers */
#define array_size(arr) (sizeof(arr) / sizeof((arr)[0]))

/* Offset of member in struct */
#ifndef offsetof
#define offsetof(type, member) ((uint32_t) & ((type *)0)->member)
#endif

/* Container_of pattern */
#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(ptr)-offsetof(type, member)))

/* Min/Max macros that evaluate arguments only once */
#define MIN(a, b)                                                              \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(b) _b = (b);                                                        \
    _a < _b ? _a : _b;                                                         \
  })

#define MAX(a, b)                                                              \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(b) _b = (b);                                                        \
    _a > _b ? _a : _b;                                                         \
  })

/* Swap macro */
#define SWAP(a, b)                                                             \
  do {                                                                         \
    typeof(a) _tmp = (a);                                                      \
    (a) = (b);                                                                 \
    (b) = _tmp;                                                                \
  } while (0)
