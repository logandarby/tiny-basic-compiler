#pragma once

// ----------------------------------------------
// PLATFORM DETECTION
//
// Sets macros based off OS and architecture
// ----------------------------------------------

#include "core.h"

// X macro definitions for OS types
#define OS_TYPES(X)                                                            \
  X(OS_UNKNOWN, "unknown")                                                     \
  X(OS_WINDOWS, "windows")                                                     \
  X(OS_LINUX, "linux")                                                         \
  X(OS_MACOS, "macos")

// X macro definitions for architecture types
#define ARCH_TYPES(X)                                                          \
  X(ARCH_UNKNOWN, "unknown")                                                   \
  X(ARCH_X86_32, "x86")                                                        \
  X(ARCH_X86_64, "x86_64")

// Generate OS enum
typedef enum {
#define X(name, str) name,
  OS_TYPES(X)
#undef X
} OS;

// Generate ARCH enum
typedef enum {
#define X(name, str) name,
  ARCH_TYPES(X)
#undef X
} ARCH;

// Target ABI enumeration
typedef enum {
  ABI_UNKNOWN = 0,
  ABI_SYSV, // System V (Linux, Unix, MacOS)
  ABI_MS,   // Microsoft (Windows)
} ABI;

typedef struct {
  OS os;
  ARCH arch;
  ABI abi;
} PlatformInfo;

#define MAX_REGISTER 6

typedef struct {
  const char *arg_r[MAX_REGISTER];     // Argument registers
  const char *scratch_r[MAX_REGISTER]; // Scratch registers
  const char *ret_r;                   // Return reg
  const char *rsp;                     // Stack pointer reg
  const char *rbp;                     // Stack base pointer reg
  const char *rip;                     // instruction reg
  const uint8_t stack_alignment;
  const uint8_t shadow_space;
  const uint8_t ptr_size;
} CallingConvention;

extern const PlatformInfo HOST_INFO;
extern const CallingConvention CC_SYSTEM_V_64;
extern const CallingConvention CC_MS_64;

const CallingConvention *
get_calling_convention(const PlatformInfo *PlatformInfo);

// Convert PlatformInfo back to target triple string
// Returns a newly allocated string that must be freed by caller
// Returns NULL on error or if any component is UNKNOWN
char *platform_info_to_triple(const PlatformInfo *info);

// Parse target triple in format: arch-os
// Example: "x86_64-linux"
// Returns PlatformInfo with parsed components inside the "info" variable
// If it errors, it returns false, and sets PlatformInfo to NULL
bool parse_target_triple(const char *triple, PlatformInfo *info);
