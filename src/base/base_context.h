#ifndef BASE_CONTEXT_H
#define BASE_CONTEXT_H

// Language Cracking
//=============================================================================

#if defined(__cplusplus)
#   define LANGUAGE_CPP 1
#else
#   define LANGUAGE_C 1
#endif

// Compiler OS/Arch Chacking
//=============================================================================

// Clang ======================================================================

#if defined(__clang__)

#   define COMPILER_CLANG 1

#   if defined(_WIN32)
#       define OS_WINDOWS 1
#   elif defined(__gnu_linux__) || defined(__linux__)
#       define OS_LINUX 1
#   elif defined(__APPLE__) && defined(__MACH__)
#       define OS_MAC 1
#   else
#       error This compiler/OS combo is not supported.
#   endif

#   if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#       define ARCH_X64 1
#   elif defined(i386) || defined(__i386) || defined(__i386__)
#       define ARCH_X86 1
#   elif defined(__aarch64__)
#       define ARCH_ARM64 1
#   elif defined(__arm__)
#       define ARCH_ARM32 1
#   else
#       error Architecture not supported.
#   endif

// MSVC OS/Arch Cracking ======================================================

#elif defined(_MSC_VER)

#   define COMPILER_MSVC 1

#   if _MSC_VER >= 1920
#       define COMPILER_MSVC_YEAR 2019
#   elif _MSC_VER >= 1910
#       define COMPILER_MSVC_YEAR 2017
#   elif _MSC_VER >= 1900
#       define COMPILER_MSVC_YEAR 2015
#   elif _MSC_VER >= 1800
#       define COMPILER_MSVC_YEAR 2013
#   elif _MSC_VER >= 1700
#       define COMPILER_MSVC_YEAR 2012
#   elif _MSC_VER >= 1600
#       define COMPILER_MSVC_YEAR 2010
#   elif _MSC_VER >= 1500
#       define COMPILER_MSVC_YEAR 2008
#   elif _MSC_VER >= 1400
#       define COMPILER_MSVC_YEAR 2005
#   else
#       define COMPILER_MSVC_YEAR 0
#   endif

#   if defined(_WIN32)
#       define OS_WINDOWS 1
#   else
#       error This compiler/OS combo is not supported.
#   endif

#   if defined(_M_AMD64)
#       define ARCH_X64 1
#   elif defined(_M_IX86)
#       define ARCH_X86 1
#   elif defined(_M_ARM64)
#       define ARCH_ARM64 1
#   elif defined(_M_ARM)
#       define ARCH_ARM32 1
#   else
#       error Architecture not supported.
#   endif

// GCC OS/Arch Cracking =======================================================

#elif defined(__GNUC__) || defined(__GNUG__)

#   define COMPILER_GCC 1

#   if defined(__gnu_linux__) || defined(__linux__)
#       define OS_LINUX 1
#   elif defined(_WIN32)
#       define OS_WINDOWS 1
#   else
#       error This compiler/OS combo is not supported.
#   endif

#   if defined(__amd64__) || defined(__amd64) || defined(__x86_64__) || defined(__x86_64)
#       define ARCH_X64 1
#   elif defined(i386) || defined(__i386) || defined(__i386__)
#       define ARCH_X86 1
#   elif defined(__aarch64__)
#       define ARCH_ARM64 1
#   elif defined(__arm__)
#       define ARCH_ARM32 1
#   else
#       error Architecture not supported.
#   endif

// TCC OS/Arch Checking =======================================================

#elif defined(__TINYC__)

#   define COMPILER_TCC 1

#   if defined(__linux__)
#       define OS_LINUX 1
#   elif defined(__windows__)
#       define OS_WINDOWS 1
#   else
#       error This OS is not supported by TCC.
#   endif

#   if defined(__x86_64__)
#       define ARCH_X64 1
#   elif defined(__i386__)
#       define ARCH_X86 1
#   elif defined(__aarch64__)
#       define ARCH_ARM64 1
#   elif defined(__arm__)
#       define ARCH_ARM32 1
#   else
#       error Architecture not supported by TCC.
#   endif

// Not supported compiler =====================================================

#else
#   error Compiler not supported.
#endif

// Arch Checking
//=============================================================================

#if defined(ARCH_X64)
#   define ARCH_64BIT 1
#elif defined(ARCH_X86)
#   define ARCH_32BIT 1
#endif

#if ARCH_ARM32 || ARCH_ARM64 || ARCH_X64 || ARCH_X86
#   define ARCH_LITTLE_ENDIAN 1
#else
#   error Endianness of this architecture not understood by context cracker.
#endif

// MinGW Checking
//=============================================================================

#ifdef __MINGW32__
#   define TOOLCHAIN_MINGW 1
#endif

// ak: Address Sanitizer Checking
//=============================================================================

#if COMPILER_MSVC
#   if defined(__SANITIZE_ADDRESS__)
#       define ASAN_ENABLED 1
#       define NO_ASAN __declspec(no_sanitize_address)
#   else
#       define NO_ASAN
#   endif
#elif COMPILER_CLANG
#   if defined(__has_feature)
#       if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__)
#           define ASAN_ENABLED 1
#       endif
#   endif
#   define NO_ASAN __attribute__((no_sanitize("address")))
#elif COMPILER_GCC
#   if defined(__SANITIZE_ADDRESS__)
#       define ASAN_ENABLED 1
#   endif
#   define NO_ASAN __attribute__((no_sanitize("address")))
#else
#   error ASAN is not defined for this compiler
#endif

// Toolchain/Environment Enums
//=============================================================================

typedef enum Context_Os
{
    Context_Os_Null,
    Context_Os_Windows,
    Context_Os_Linux,
    Context_Os_Mac,
    Context_Os_COUNT,
#if OS_WINDOWS
    Context_Os_CURRENT = Context_Os_Windows,
#elif OS_LINUX
    Context_Os_CURRENT = Context_Os_Linux,
#elif OS_MAC
    Context_Os_CURRENT = Context_Os_Mac,
#else
    Context_Os_CURRENT = Context_Os_Null,
#endif
}
Context_Os;

typedef enum Context_Arch
{
    Context_Arch_Null,
    Context_Arch_X64,
    Context_Arch_X86,
    Context_Arch_Arm64,
    Context_Arch_Arm32,
    Context_Arch_COUNT,
#if ARCH_X64
    Context_Arch_CURRENT = Context_Arch_X64,
#elif ARCH_X86
    Context_Arch_CURRENT = Context_Arch_X86,
#elif ARCH_ARM64
    Context_Arch_CURRENT = Context_Arch_Arm64,
#elif ARCH_ARM32
    Context_Arch_CURRENT = Context_Arch_Arm32,
#else
    Context_Arch_CURRENT = Context_Arch_Null,
#endif
}
Context_Arch;

typedef enum Context_Compiler
{
    Context_Compiler_Null,
    Context_Compiler_Msvc,
    Context_Compiler_Gcc,
    Context_Compiler_Clang,
    Context_Compiler_COUNT,
#if COMPILER_MSVC
    Context_Compiler_CURRENT = Context_Compiler_Msvc,
#elif COMPILER_GCC
    Context_Compiler_CURRENT = Context_Compiler_Gcc,
#elif COMPILER_CLANG
    Context_Compiler_CURRENT = Context_Compiler_Clang,
#else
    Context_Compiler_CURRENT = Context_Compiler_Null,
#endif
}
Context_Compiler;

#endif // BASE_CONTEXT_H
