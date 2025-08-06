#pragma once

// ----------------------------------------------
// PLATFORM DETECTION
//
// Sets macros based off OS and architecture
// ----------------------------------------------

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
  OS os;
  ARCH arch;
  ABI abi;
} HostInfo;

static const HostInfo HOST_INFO = {
#if defined(_WIN32) || defined(_WIN64_)
    .os = OS_WINDOWS,
    .abi = ABI_MS,
#elif defined(__APPLE__)
    .os = OS_MACOS,
    .abi = ABI_SYSV,
#elif defined(__linux__)
    .os = OS_LINUX,
    .abi = ABI_SYSV,
#else
    .os = OS_UNKNOWN,
    .abi = ABI_UNKNOWN,
#endif

#if defined(__x86_64__) || defined(_M_X64)
    .arch = ARCH_X86_64,
#elif defined(__i386__) || defined(_M_IX86)
    .arch = ARCH_X86_32,
#else
    .arch = ARCH_UNKNOWN,
#endif

};
