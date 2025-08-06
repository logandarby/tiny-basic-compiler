#pragma once

// Defines common macros, logging and assertion
// All nasty macros will be stored here (To contain the damage!)
//
// Options:
//  - DZ_ENABLE_DEBUGBREAK = 1  - enables debug breaking
//  - DZ_ENABLE_LOGS = 1        - Allows the program to log (Error
//  logging is not affected by this)
//  - DZ_ENABLE_ASSERTS = 1     - Allows the program to assert
//  conditions
//
// These are all enabled when debugging is enabled using the DZ_DEBUG
// flag

#include "../core/compiler.h"
#include <execinfo.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef DEBUG
#define DZ_DEBUG
#endif

#ifdef DZ_DEBUG
#define DZ_ENABLE_ASSERTS 1
#define DZ_ENABLE_DEBUGBREAK 1
#define DZ_ENABLE_LOGS 1
#endif

// Terminal Colours
extern const char *KNRM;
extern const char *KRED;
extern const char *KGRN;
extern const char *KYEL;
extern const char *KWHT;
extern const char *KBLU;
extern const char *KMAG;
extern const char *KCYN;

// More readable strcmp == 0 call
extern bool str_eq(const char *s1, const char *s2, uint32_t n);

// More readable memvmp == 0
extern bool mem_eq(const void *s1, const void *s2, uint32_t s1_size,
                   uint32_t s2_size);

// Util Macros
#define DZ_EXPAND_MACRO(x) x
#define DZ_STRINGIFY(x) #x

// LOGGING - Trace, Info, Warn, and Error
// Logs with different levels. Also prints a formatted date and time
// with pretty colors Arguments: A format string and its arguments --
// similar to printf DZ_WARNNO and DZ_ERRNO are the same as the Warn
// and Error levels, but they also log the current error number as
// defined in errno.h
#if DZ_ENABLE_LOGS == 1
#define DZ_TRACE(...)                                                          \
  dz_impl_log(stdout, DzErrorLevel_TRACE, false, __FILE__, __func__, __LINE__, \
              __VA_ARGS__)
#define DZ_INFO(...)                                                           \
  dz_impl_log(stdout, DzErrorLevel_INFO, false, __FILE__, __func__, __LINE__,  \
              __VA_ARGS__)
#define DZ_WARN(...)                                                           \
  dz_impl_log(stdout, DzErrorLevel_WARN, false, __FILE__, __func__, __LINE__,  \
              __VA_ARGS__)
#define DZ_WARNNO(...)                                                         \
  dz_impl_log(stdout, DzErrorLevel_WARN, true, __FILE__, __func__, __LINE__,   \
              __VA_ARGS__)
#else
#define DZ_TRACE(...)
#define DZ_INFO(...)
#define DZ_WARN(...)
#define DZ_WARNNO(...)
#endif

#define DZ_ERRNO(...) DZ_ERRORNO(__VA_ARGS__)
#define DZ_ERROR(...)                                                          \
  dz_impl_log(stderr, DzErrorLevel_ERROR, false, __FILE__, __func__, __LINE__, \
              __VA_ARGS__)
#define DZ_ERRORNO(...)                                                        \
  dz_impl_log(stderr, DzErrorLevel_ERROR, true, __FILE__, __func__, __LINE__,  \
              __VA_ARGS__)

// THROW -- Logs an error message, prints backtrace, and terminates the program
// Arguments: A format string and its arguments -- similar to printf
#define DZ_THROW(...) dz_impl_throw(__FILE__, __func__, __LINE__, __VA_ARGS__)

// DEBUGBREAK -- Breaks when encountered
#if DZ_ENABLE_DEBUGBREAK == 1
#include <signal.h>
#define DZ_DEBUGBREAK(...) raise(SIGTRAP)
#else
#define DZ_DEBUGBREAK(...)
#endif

// ASSERTIONS -- Asserts a condition, optionally with a message
#if DZ_ENABLE_ASSERTS == 1
// Asserts a condition.
// Arguments:
// Condition - The condition to assert
// Message (Optional) - A message to display when assertion fails
#define DZ_ASSERT(...)                                                         \
  DZ_EXPAND_MACRO(DZ_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(_, __VA_ARGS__))
// Asserts a constant condition at compile time
// Arguments:
//  - Condition: The constant, compile time condition to assert
//  - Message: The message to display to the user
#define DZ_STATIC_ASSERT(cond, msg)                                            \
  ({                                                                           \
    static_assert(cond, msg);                                                  \
    0;                                                                         \
  })
// Internal macro impl
#define DZ_INTERNAL_ASSERT_WITH_MSG(type, check, ...)                          \
  dz_impl_assert_msg(__FILE__, __func__, __LINE__, DZ_STRINGIFY(check), check, \
                     __VA_ARGS__)
#define DZ_INTERNAL_ASSERT_NO_MSG(type, check)                                 \
  dz_impl_assert_msg(__FILE__, __func__, __LINE__, DZ_STRINGIFY(check), check, \
                     NULL)
#define DZ_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
#define DZ_INTERNAL_ASSERT_GET_MACRO(...)                                      \
  DZ_EXPAND_MACRO(DZ_INTERNAL_ASSERT_GET_MACRO_NAME(                           \
      __VA_ARGS__, DZ_INTERNAL_ASSERT_WITH_MSG, DZ_INTERNAL_ASSERT_NO_MSG))
#else
#define DZ_ASSERT(...)
#endif

// Implementation Details

typedef enum DzErrorLevel {
  DzErrorLevel_INFO,
  DzErrorLevel_TRACE,
  DzErrorLevel_WARN,
  DzErrorLevel_ERROR,
} DzErrorLevel;

// Asserts with file, line number, and condition information.
// Optionally takes a message. You can leave it as NULL if you don't
// want one
extern void dz_impl_assert_msg(const char *filename, const char *function_name,
                               const int line_number,
                               const char *condition_string, bool condition,
                               const char *msg, ...) FORMAT_PRINTF(6, 7);

// Throws an error with backtrace and terminates the program
extern void dz_impl_throw(const char *filename, const char *function_name,
                          const int line_number, const char *msg, ...)
    FORMAT_PRINTF(4, 5);

extern void dz_impl_log(FILE *stream, DzErrorLevel error_level, bool show_errno,
                        const char *filename, const char *function_name,
                        const int line_number, const char *msg, ...)
    FORMAT_PRINTF(7, 8);
