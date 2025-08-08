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
