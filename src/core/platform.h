#pragma once

// ----------------------------------------------
// PLATFORM DETECTION
//
// Sets macros based off OS and architecture
// ----------------------------------------------

#include "core.h"

typedef enum {
  OS_UNKNOWN = 0,
  OS_WINDOWS,
  OS_LINUX,
  OS_MACOS,
} OS;

typedef enum {
  ARCH_UNKNOWN = 0,
  ARCH_X86_32,
  ARCH_X86_64,
} ARCH;

// Target ABI enumeration
typedef enum {
  ABI_UNKNOWN = 0,
  ABI_SYSV, // System V (Linux, Unix, MacOS)
  ABI_MS,   // Microsoft (Windows)
} ABI;

typedef struct {
  const OS os;
  const ARCH arch;
  const ABI abi;
} PlatformInfo;

#define MAX_REGISTER 6

typedef struct {
  const char *argument_regs[MAX_REGISTER];
  const char *scratch_regs[MAX_REGISTER];
  const char *return_reg;
  const char *stack_reg;
  const char *base_reg;
  const char *inst_reg;
  const uint8_t stack_alignment;
  const uint8_t shadow_space;
  const uint8_t ptr_size;
} CallingConvention;

extern const PlatformInfo HOST_INFO;
extern const CallingConvention CC_SYSTEM_V_64;
extern const CallingConvention CC_MS_64;

const CallingConvention *
get_calling_convention(const PlatformInfo *PlatformInfo);
