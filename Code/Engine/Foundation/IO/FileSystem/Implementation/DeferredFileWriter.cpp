#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>

plDeferredFileWriter::plDeferredFileWriter()
  : m_Writer(&m_Storage)
{
}

void plDeferredFileWriter::SetOutput(plStringView sFileToWriteTo, bool bOnlyWriteIfDifferent)
{
  m_bOnlyWriteIfDifferent = bOnlyWriteIfDifferent;
  m_sOutputFile = sFileToWriteTo;
}

plResult plDeferredFileWriter::WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite)
{
  PL_ASSERT_DEBUG(!m_sOutputFile.IsEmpty(), "Output file has not been configured");

  return m_Writer.WriteBytes(pWriteBuffer, uiBytesToWrite);
}

plResult plDeferredFileWriter::Close(bool* out_pWasWrittenTo /*= nullptr*/)
{
  if (out_pWasWrittenTo)
  {
    *out_pWasWrittenTo = false;
  }

  if (m_bAlreadyClosed)
    return PL_SUCCESS;

  if (m_sOutputFile.IsEmpty())
    return PL_FAILURE;

  m_bAlreadyClosed = true;

  if (m_bOnlyWriteIfDifferent)
  {
    plFileReader fileIn;
    if (fileIn.Open(m_sOutputFile).Succeeded() && fileIn.GetFileSize() == m_Storage.GetStorageSize64())
    {
      plUInt8 tmp1[1024 * 4];
      plUInt8 tmp2[1024 * 4];

      plMemoryStreamReader storageReader(&m_Storage);

      while (true)
      {
        const plUInt64 readBytes1 = fileIn.ReadBytes(tmp1, PL_ARRAY_SIZE(tmp1));
        const plUInt64 readBytes2 = storageReader.ReadBytes(tmp2, PL_ARRAY_SIZE(tmp2));

        if (readBytes1 != readBytes2)
          goto write_data;

        if (readBytes1 == 0)
          break;

        if (plMemoryUtils::RawByteCompare(tmp1, tmp2, plMath::SafeConvertToSizeT(readBytes1)) != 0)
          goto write_data;
      }

      // content is already the same as what we would write -> skip the write (do not modify file write date)
      return PL_SUCCESS;
    }
  }

write_data:
  plFileWriter file;
  PL_SUCCEED_OR_RETURN(file.Open(m_sOutputFile, 0)); // use the minimum cache size, we want to pass data directly through to disk

  if (out_pWasWrittenTo)
  {
    *out_pWasWrittenTo = true;
  }

  m_sOutputFile.Clear();
  return m_Storage.CopyToStream(file);
}

void plDeferredFileWriter::Discard()
{
  m_sOutputFile.Clear();
}


