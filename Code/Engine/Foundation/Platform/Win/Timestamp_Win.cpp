#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS)

#include <Foundation/Time/Timestamp.h>

// Helper function to shift windows file time into Unix epoch (in microseconds).
plInt64 FileTimeToEpoch(FILETIME fileTime)
{
  ULARGE_INTEGER currentTime;
  currentTime.LowPart = fileTime.dwLowDateTime;
  currentTime.HighPart = fileTime.dwHighDateTime;

  plInt64 iTemp = currentTime.QuadPart / 10;
  iTemp -= 11644473600000000LL;
  return iTemp;
}

// Helper function to shift Unix epoch (in microseconds) into windows file time.
FILETIME EpochToFileTime(plInt64 iFileTime)
{
  plInt64 iTemp = iFileTime + 11644473600000000LL;
  iTemp *= 10;

  FILETIME fileTime;
  ULARGE_INTEGER currentTime;
  currentTime.QuadPart = iTemp;
  fileTime.dwLowDateTime = currentTime.LowPart;
  fileTime.dwHighDateTime = currentTime.HighPart;
  return fileTime;
}

const plTimestamp plTimestamp::CurrentTimestamp()
{
  FILETIME fileTime;
  GetSystemTimeAsFileTime(&fileTime);
  return plTimestamp::MakeFromInt(FileTimeToEpoch(fileTime), plSIUnitOfTime::Microsecond);
}

const plTimestamp plDateTime::GetTimestamp() const
{
  SYSTEMTIME st;
  FILETIME fileTime;
  memset(&st, 0, sizeof(SYSTEMTIME));
  st.wYear = (WORD)m_iYear;
  st.wMonth = m_uiMonth;
  st.wDay = m_uiDay;
  st.wDayOfWeek = m_uiDayOfWeek;
  st.wHour = m_uiHour;
  st.wMinute = m_uiMinute;
  st.wSecond = m_uiSecond;
  st.wMilliseconds = (WORD)(m_uiMicroseconds / 1000);
  BOOL res = SystemTimeToFileTime(&st, &fileTime);
  plTimestamp timestamp;
  if (res != 0)
    timestamp = plTimestamp::MakeFromInt(FileTimeToEpoch(fileTime), plSIUnitOfTime::Microsecond);

  return timestamp;
}

plResult plDateTime::SetFromTimestamp(plTimestamp timestamp)
{
  FILETIME fileTime = EpochToFileTime(timestamp.GetInt64(plSIUnitOfTime::Microsecond));

  SYSTEMTIME st;
  BOOL res = FileTimeToSystemTime(&fileTime, &st);
  if (res == 0)
    return PL_FAILURE;

  m_iYear = (plInt16)st.wYear;
  m_uiMonth = (plUInt8)st.wMonth;
  m_uiDay = (plUInt8)st.wDay;
  m_uiDayOfWeek = (plUInt8)st.wDayOfWeek;
  m_uiHour = (plUInt8)st.wHour;
  m_uiMinute = (plUInt8)st.wMinute;
  m_uiSecond = (plUInt8)st.wSecond;
  m_uiMicroseconds = plUInt32(st.wMilliseconds * 1000);
  return PL_SUCCESS;
}

#endif


