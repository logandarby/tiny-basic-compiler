#include "platform.h"
#include "dz_debug.h"
#include <stdlib.h>

const CallingConvention CC_SYSTEM_V_64 = {
    .argument_regs = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"},
    .scratch_regs = {"r10", "r11"},
    .return_reg = "rax",
    .stack_reg = "rsp",
    .base_reg = "rbp",
    .inst_reg = "rip",
    .stack_alignment = 16,
    .shadow_space = 0,
    .ptr_size = 8,
};

const CallingConvention CC_MS_64 = {
    .argument_regs = {"rcx", "rdx", "r8", "r9"},
    .scratch_regs = {"r10", "r11"},
    .return_reg = "rax",
    .stack_reg = "rsp",
    .base_reg = "rbp",
    .inst_reg = "rip",
    .stack_alignment = 16,
    .shadow_space = 32,
    .ptr_size = 8,
};

ABI get_abi_from_os(const OS os) {
  switch (os) {
  case OS_WINDOWS:
    return ABI_MS;
  case OS_MACOS:
  case OS_LINUX:
    return ABI_SYSV;
  case OS_UNKNOWN:
    return ABI_UNKNOWN;
  }
  return ABI_UNKNOWN;
}

const PlatformInfo HOST_INFO = {
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

const CallingConvention *get_calling_convention(const PlatformInfo *info) {
  if (info->arch != ARCH_X86_64) {
    DZ_ERROR("32-bit Architecture is not supported");
    exit(EXIT_FAILURE);
  }
  if (info->abi == ABI_MS) {
    return &CC_MS_64;
  } else if (info->abi == ABI_SYSV) {
    return &CC_SYSTEM_V_64;
  } else {
    DZ_ERROR("OS/ABI is not supported.");
    exit(EXIT_FAILURE);
  }
}

// Generate OS parsing function
static OS _parse_os(const char *os_str) {
  if (os_str == NULL)
    return OS_UNKNOWN;

#define X(name, str)                                                           \
  if (strcmp(os_str, str) == 0)                                                \
    return name;
  OS_TYPES(X)
#undef X
  return OS_UNKNOWN;
}

// Generate ARCH parsing function
static ARCH _parse_arch(const char *arch_str) {
  if (arch_str == NULL)
    return ARCH_UNKNOWN;

#define X(name, str)                                                           \
  if (strcmp(arch_str, str) == 0)                                              \
    return name;
  ARCH_TYPES(X)
#undef X

  // Handle common aliases
  if (strcmp(arch_str, "i386") == 0)
    return ARCH_X86_32;
  if (strcmp(arch_str, "x64") == 0)
    return ARCH_X86_64;

  return ARCH_UNKNOWN;
}

// Generate OS to string function
static const char *_os_to_string(OS os) {
  switch (os) {
#define X(name, str)                                                           \
  case name:                                                                   \
    return str;
    OS_TYPES(X)
#undef X
  }
  return "unknown";
}

// Generate ARCH to string function
static const char *_arch_to_string(ARCH arch) {
  switch (arch) {
#define X(name, str)                                                           \
  case name:                                                                   \
    return str;
    ARCH_TYPES(X)
#undef X
  }
  return "unknown";
}

bool parse_target_triple(const char *triple, PlatformInfo *info) {
  if (triple == NULL) {
    info = NULL;
    return false;
  }

  // Make a copy since strtok modifies the string
  char *triple_copy = xmalloc(strlen(triple) + 1);
  strcpy(triple_copy, triple);

  // Parse the two components
  char *arch_str = strtok(triple_copy, "-");
  char *os_str = strtok(NULL, "-");

  if (!arch_str || !os_str) {
    free(triple_copy);
    info = NULL;
    return false;
  }

  // Parse each component
  ARCH arch = _parse_arch(arch_str);
  OS os = _parse_os(os_str);

  free(triple_copy);

  *info = (PlatformInfo){
      .arch = arch,
      .os = os,
      .abi = get_abi_from_os(os),
  };
  return true;
}

char *platform_info_to_triple(const PlatformInfo *info) {
  if (info == NULL) {
    return NULL;
  }

  const char *arch_str = _arch_to_string(info->arch);
  const char *os_str = _os_to_string(info->os);

  if (arch_str == NULL || os_str == NULL) {
    return NULL;
  }

  // Allocate and format the result string
  size_t len =
      strlen(arch_str) + strlen(os_str) + 2; // +1 for dash, +1 for null
  char *result = malloc(len);
  if (result == NULL) {
    return NULL;
  }

  snprintf(result, len, "%s-%s", arch_str, os_str);
  return result;
}
