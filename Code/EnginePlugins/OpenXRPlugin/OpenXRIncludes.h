#pragma once

#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <d3d11_1.h>
#define XR_USE_GRAPHICS_API_D3D11
#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
#  define XR_USE_PLATFORM_WIN32
#endif
#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
#include <openxr/openxr_platform_defines.h>

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
#  include <openxr/openxr_msft_holographic_remoting.h>
#endif
