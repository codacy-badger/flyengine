#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/FileSystem/Implementation/FileReaderWriterBase.h>
#include <Foundation/IO/Stream.h>

/// \brief The default class to use to read data from a file, implements the plStreamReader interface.
///
/// This file reader buffers reads up to a certain amount of bytes (configurable).
/// It closes the file automatically once it goes out of scope.
class PL_FOUNDATION_DLL plFileReader : public plFileReaderBase
{
  PL_DISALLOW_COPY_AND_ASSIGN(plFileReader);

public:
  /// \brief Constructor, does nothing.
  plFileReader()

    = default;

  /// \brief Destructor, closes the file, if it is still open (RAII).
  ~plFileReader() { Close(); }

  /// \brief Opens the given file for reading. Returns PL_SUCCESS if the file could be opened. A cache is created to speed up small reads.
  ///
  /// You should typically not disable bAllowFileEvents, unless you need to prevent recursive file events,
  /// which is only the case, if you are doing file accesses from within a File Event Handler.
  plResult Open(plStringView sFile, plUInt32 uiCacheSize = 1024 * 64, plFileShareMode::Enum fileShareMode = plFileShareMode::Default,
    bool bAllowFileEvents = true);

  /// \brief Closes the file, if it is open.
  void Close();

  /// \brief Attempts to read the given number of bytes into the buffer. Returns the actual number of bytes read.
  virtual plUInt64 ReadBytes(void* pReadBuffer, plUInt64 uiBytesToRead) override;

private:
  plUInt64 m_uiBytesCached = 0;
  plUInt64 m_uiCacheReadPosition = 0;
  plDynamicArray<plUInt8> m_Cache;
  bool m_bEOF = true;
};
