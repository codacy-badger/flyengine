#include <Foundation/FoundationPCH.h>

#if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)

#  include <Foundation/Basics/Platform/Win/HResultUtils.h>
#  include <Foundation/Strings/StringBuilder.h>
#  include <Foundation/Strings/StringConversion.h>

PLASMA_FOUNDATION_DLL plString plHRESULTtoString(plMinWindows::HRESULT result)
{
  wchar_t buffer[4096];
  if (::FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM,
        nullptr,
        result,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
        buffer,
        PLASMA_ARRAY_SIZE(buffer),
        nullptr) == 0)
  {
    return {};
  }

  // Com error tends to put /r/n at the end. Remove it.
  plStringBuilder message(plStringUtf8(&buffer[0]).GetData());
  message.ReplaceAll("\n", "");
  message.ReplaceAll("\r", "");

  return message;
}

#endif



PLASMA_STATICLINK_FILE(Foundation, Foundation_Basics_Platform_Win_HResultUtils);
