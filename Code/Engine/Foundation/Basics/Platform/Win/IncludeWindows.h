#pragma once

#include <Foundation/Basics.h>

#define PL_INCLUDED_WINDOWS_H 1

#if PL_ENABLED(PL_PLATFORM_WINDOWS)
// this is important for code that wants to include winsock2.h later on
#  define _WINSOCKAPI_ /* Prevent inclusion of winsock.h in windows.h */

// already includes Windows.h, but defines important other things first
//#include <winsock2.h>

#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <Windows.h>

// unset windows macros
#  undef min
#  undef max
#  undef GetObject
#  undef ERROR
#  undef DeleteFile
#  undef CopyFile
#  undef DispatchMessage
#  undef PostMessage
#  undef SendMessage
#  undef OPAQUE
#  undef SetPort

#  include <Foundation/Basics/Platform/Win/MinWindows.h>

namespace plMinWindows
{
  template <>
  struct ToNativeImpl<HINSTANCE>
  {
    using type = ::HINSTANCE;
    static PL_ALWAYS_INLINE ::HINSTANCE ToNative(HINSTANCE hInstance) { return reinterpret_cast<::HINSTANCE>(hInstance); }
  };

  template <>
  struct ToNativeImpl<HWND>
  {
    using type = ::HWND;
    static PL_ALWAYS_INLINE ::HWND ToNative(HWND hWnd) { return reinterpret_cast<::HWND>(hWnd); }
  };

  template <>
  struct FromNativeImpl<::HWND>
  {
    using type = HWND;
    static PL_ALWAYS_INLINE HWND FromNative(::HWND hWnd) { return reinterpret_cast<HWND>(hWnd); }
  };

  template <>
  struct FromNativeImpl<::HINSTANCE>
  {
    using type = HINSTANCE;
    static PL_ALWAYS_INLINE HINSTANCE FromNative(::HINSTANCE hInstance) { return reinterpret_cast<HINSTANCE>(hInstance); }
  };
} // namespace plMinWindows
#endif
