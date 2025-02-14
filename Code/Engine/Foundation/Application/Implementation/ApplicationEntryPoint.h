#pragma once

#include <Foundation/Basics.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Application/Implementation/Win/ApplicationEntryPoint_win.h>

#elif PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)

#  include <Foundation/Application/Implementation/uwp/ApplicationEntryPoint_uwp.h>

#elif PL_ENABLED(PL_PLATFORM_OSX) || PL_ENABLED(PL_PLATFORM_LINUX)

#  include <Foundation/Application/Implementation/Posix/ApplicationEntryPoint_posix.h>

#elif PL_ENABLED(PL_PLATFORM_ANDROID)

#  include <Foundation/Application/Implementation/Android/ApplicationEntryPoint_android.h>

#else
#  error "Missing definition of platform specific entry point!"
#endif
