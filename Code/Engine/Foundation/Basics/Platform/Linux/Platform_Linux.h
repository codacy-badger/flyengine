#pragma once

#if PL_DISABLED(PL_PLATFORM_LINUX) && PL_DISABLED(PL_PLATFORM_ANDROID)
#  error "This header should only be included on Linux"
#endif

#include <cstdio>
#include <malloc.h>
#include <pthread.h>
#include <stdarg.h>
#include <sys/time.h>
#include <unistd.h>

// unset common macros
#ifdef min
#  undef min
#endif
#ifdef max
#  undef max
#endif

#include <Foundation/Basics/Compiler/Clang/Clang.h>
#include <Foundation/Basics/Compiler/GCC/GCC.h>
#include <Foundation/Basics/Compiler/MSVC/MSVC.h>

#undef PL_PLATFORM_LITTLE_ENDIAN
#define PL_PLATFORM_LITTLE_ENDIAN PL_ON
