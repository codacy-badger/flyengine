#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlParser.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/ConversionUtils.h>

plOpenDdlParser::plOpenDdlParser()
{
  m_pLogInterface = nullptr;
  m_bHadFatalParsingError = false;
}

void plOpenDdlParser::SetCacheSize(plUInt32 uiSizeInKB)
{
  m_Cache.SetCountUninitialized(plMath::Max<plUInt32>(1, uiSizeInKB) * 1024);
  m_TempString.SetCountUninitialized(plMath::Max<plUInt32>(1, uiSizeInKB) * 1024);

  m_pBoolCache = reinterpret_cast<bool*>(m_Cache.GetData());
  m_pInt8Cache = reinterpret_cast<plInt8*>(m_Cache.GetData());
  m_pInt16Cache = reinterpret_cast<plInt16*>(m_Cache.GetData());
  m_pInt32Cache = reinterpret_cast<plInt32*>(m_Cache.GetData());
  m_pInt64Cache = reinterpret_cast<plInt64*>(m_Cache.GetData());
  m_pUInt8Cache = reinterpret_cast<plUInt8*>(m_Cache.GetData());
  m_pUInt16Cache = reinterpret_cast<plUInt16*>(m_Cache.GetData());
  m_pUInt32Cache = reinterpret_cast<plUInt32*>(m_Cache.GetData());
  m_pUInt64Cache = reinterpret_cast<plUInt64*>(m_Cache.GetData());
  m_pFloatCache = reinterpret_cast<float*>(m_Cache.GetData());
  m_pDoubleCache = reinterpret_cast<double*>(m_Cache.GetData());
}


// Extension to default OpenDDL: We allow ':' and '.' to appear in identifier names
bool IsDdlIdentifierCharacter(plUInt32 uiByte)
{
  return ((uiByte >= 'a' && uiByte <= 'z') || (uiByte >= 'A' && uiByte <= 'Z') || (uiByte == '_') || (uiByte >= '0' && uiByte <= '9') || (uiByte == ':') || (uiByte == '.'));
}

void plOpenDdlParser::SetInputStream(plStreamReader& stream, plUInt32 uiFirstLineOffset /*= 0*/)
{
  PL_ASSERT_DEV(m_StateStack.IsEmpty(), "OpenDDL Parser cannot be restarted");

  m_pInput = &stream;

  m_bSkippingMode = false;
  m_uiCurLine = 1 + uiFirstLineOffset;
  m_uiCurColumn = 0;
  m_uiCurByte = '\0';
  m_uiNumCachedPrimitives = 0;

  // get into a valid state
  m_uiNextByte = ' ';
  ReadCharacterSkipComments();

  // go to the start of the document, skip any comments and whitespace that the document might start with
  SkipWhitespace();

  if (m_uiCurByte == '\0')
  {
    // document is empty
    m_StateStack.Clear();
  }
  else if (IsDdlIdentifierCharacter(m_uiCurByte))
  {
    m_StateStack.PushBack(State::Finished);
    m_StateStack.PushBack(State::Idle);

    if (m_Cache.IsEmpty())
      SetCacheSize(4);
  }
  else
  {
    ParsingError("Document starts with an invalid character", true);
  }
}

bool plOpenDdlParser::ContinueParsing()
{
  if (m_uiCurByte == '\0')
  {
    if (m_StateStack.GetCount() == 1)
    {
      ParsingError("More objects were closed than opened.", true);
    }

    // there's always the main Idle state on the top of the stack when everything went fine
    if (m_StateStack.GetCount() > 2)
    {
      ParsingError("End of the document reached without closing all objects.", true);
    }

    m_StateStack.Clear();

    // nothing left to do
    return false;
  }

  switch (m_StateStack.PeekBack().m_State)
  {
    case State::Finished:
      ParsingError("More objects were closed than opened.", true);
      return false;

    case State::Idle:
      ContinueIdle();
      return true;

    case State::ReadingBool:
      ContinueBool();
      return true;

    case State::ReadingInt8:
    case State::ReadingInt16:
    case State::ReadingInt32:
    case State::ReadingInt64:
    case State::ReadingUInt8:
    case State::ReadingUInt16:
    case State::ReadingUInt32:
    case State::ReadingUInt64:
      ContinueInt();
      return true;

    case State::ReadingFloat:
    case State::ReadingDouble:
      ContinueFloat();
      return true;

    case State::ReadingString:
      ContinueString();
      return true;

    default:
      PL_REPORT_FAILURE("Unknown State in OpenDDL parser state machine.");
      return false;
  }
}

plResult plOpenDdlParser::ParseAll()
{
  while (ContinueParsing())
  {
  }

  return m_bHadFatalParsingError ? PL_FAILURE : PL_SUCCESS;
}

void plOpenDdlParser::SkipRestOfObject()
{
  PL_ASSERT_DEBUG(!m_bSkippingMode, "Skipping mode is in an invalid state.");

  m_bSkippingMode = true;

  const plUInt32 iSkipToStackHeight = m_StateStack.GetCount() - 1;

  while (m_StateStack.GetCount() > iSkipToStackHeight)
    ContinueParsing();

  m_bSkippingMode = false;
}


void plOpenDdlParser::StopParsing()
{
  m_uiCurByte = '\0';
  m_StateStack.Clear();
}

void plOpenDdlParser::ParsingError(plStringView sMessage, bool bFatal)
{
  if (bFatal)
    plLog::Error(m_pLogInterface, "Line {0} ({1}): {2}", m_uiCurLine, m_uiCurColumn, sMessage);
  else
    plLog::Warning(m_pLogInterface, sMessage);

  OnParsingError(sMessage, bFatal, m_uiCurLine, m_uiCurColumn);

  if (bFatal)
  {
    // prevent further error messages
    StopParsing();
    m_bHadFatalParsingError = true;
  }
}


void plOpenDdlParser::ReadNextByte()
{
  m_pInput->ReadBytes(&m_uiNextByte, sizeof(plUInt8));

  if (m_uiNextByte == '\n')
  {
    ++m_uiCurLine;
    m_uiCurColumn = 0;
  }
  else
    ++m_uiCurColumn;
}

bool plOpenDdlParser::ReadCharacter()
{
  m_uiCurByte = m_uiNextByte;

  m_uiNextByte = '\0';
  ReadNextByte();

  return m_uiCurByte != '\0';
}

bool plOpenDdlParser::ReadCharacterSkipComments()
{
  m_uiCurByte = m_uiNextByte;

  m_uiNextByte = '\0';
  ReadNextByte();

  // skip comments
  if (m_uiCurByte == '/')
  {
    // line comment, read till line break
    if (m_uiNextByte == '/')
    {
      while (m_uiNextByte != '\0' && m_uiNextByte != '\n')
      {
        m_uiNextByte = '\0';
        ReadNextByte();
      }

      ReadCharacterSkipComments();
    }
    else if (m_uiNextByte == '*') // block comment, read till */
    {
      m_uiNextByte = ' ';

      while (m_uiNextByte != '\0' && (m_uiCurByte != '*' || m_uiNextByte != '/'))
      {
        m_uiCurByte = m_uiNextByte;

        m_uiNextByte = '\0';
        ReadNextByte();
      }

      // replace the current end-comment by whitespace
      m_uiCurByte = ' ';
      m_uiNextByte = ' ';

      ReadCharacterSkipComments();
      ReadCharacterSkipComments(); // might trigger another comment skipping
    }
  }

  return m_uiCurByte != '\0';
}

void plOpenDdlParser::SkipWhitespace()
{
  do
  {
    m_uiCurByte = '\0';

    if (!ReadCharacterSkipComments())
      return; // stop when end of stream is encountered
  } while (plStringUtils::IsWhiteSpace(m_uiCurByte));
}


void plOpenDdlParser::ContinueIdle()
{
  switch (m_uiCurByte)
  {
    case '}': // end of current object
      SkipWhitespace();

      m_StateStack.PopBack();

      if (!m_bSkippingMode)
      {
        OnEndObject();
      }
      return;

    default:
    {
      plUInt32 uiIdTypeLen = 0;
      ReadIdentifier(m_szIdentifierType, uiIdTypeLen);

      if (uiIdTypeLen == 0)
      {
        ParsingError("Object does not start with a valid type name", true);
        return;
      }

      m_szIdentifierName[0] = '\0';
      bool bGlobalName = false;

      if (m_uiCurByte == '%' || m_uiCurByte == '$')
      {
        bGlobalName = m_uiCurByte == '$';

        if (!ReadCharacterSkipComments())
          return;

        plUInt32 uiIdNameLen = 0;
        ReadIdentifier(m_szIdentifierName, uiIdNameLen);

        if (uiIdNameLen == 0)
        {
          ParsingError("Object name is empty", true);
          return;
        }
      }

      if (m_uiCurByte != '{')
      {
        ParsingError("Expected a '{' after object type name", true);
        return;
      }

      SkipWhitespace();

      // unsigned int types
      if (m_szIdentifierType[0] == 'u')
      {
        // support for 'uint' is an extension to OpenDDL
        // support for u1, u2, u3, u4 for  8 Bit, 16 Bit, 32 Bit, 64 Bit is an extension to OpenDDL

        if (plStringUtils::IsEqual((const char*)m_szIdentifierType, "u1") ||
            plStringUtils::IsEqual((const char*)m_szIdentifierType, "unsigned_int8") ||
            plStringUtils::IsEqual((const char*)m_szIdentifierType, "uint8"))
        {
          m_StateStack.PushBack(State::ReadingUInt8);

          if (!m_bSkippingMode)
          {
            OnBeginPrimitiveList(plOpenDdlPrimitiveType::UInt8, (const char*)m_szIdentifierName, bGlobalName);
          }
          return;
        }

        if (plStringUtils::IsEqual((const char*)m_szIdentifierType, "u3") ||
            plStringUtils::IsEqual((const char*)m_szIdentifierType, "unsigned_int32") ||
            plStringUtils::IsEqual((const char*)m_szIdentifierType, "uint32"))
        {
          m_StateStack.PushBack(State::ReadingUInt32);

          if (!m_bSkippingMode)
          {
            OnBeginPrimitiveList(plOpenDdlPrimitiveType::UInt32, (const char*)m_szIdentifierName, bGlobalName);
          }
          return;
        }

        if (plStringUtils::IsEqual((const char*)m_szIdentifierType, "u2") ||
            plStringUtils::IsEqual((const char*)m_szIdentifierType, "unsigned_int16") ||
            plStringUtils::IsEqual((const char*)m_szIdentifierType, "uint16"))
        {
          m_StateStack.PushBack(State::ReadingUInt16);

          if (!m_bSkippingMode)
          {
            OnBeginPrimitiveList(plOpenDdlPrimitiveType::UInt16, (const char*)m_szIdentifierName, bGlobalName);
          }
          return;
        }

        if (plStringUtils::IsEqual((const char*)m_szIdentifierType, "u4") ||
            plStringUtils::IsEqual((const char*)m_szIdentifierType, "unsigned_int64") ||
            plStringUtils::IsEqual((const char*)m_szIdentifierType, "uint64"))
        {
          m_StateStack.PushBack(State::ReadingUInt64);

          if (!m_bSkippingMode)
          {
            OnBeginPrimitiveList(plOpenDdlPrimitiveType::UInt64, (const char*)m_szIdentifierName, bGlobalName);
          }
          return;
        }
      }
      else if (m_szIdentifierType[0] == 'i') // int types
      {
        // support for i1, i2, i3, i4 for  8 Bit, 16 Bit, 32 Bit, 64 Bit is an extension to OpenDDL

        if (plStringUtils::IsEqual((const char*)m_szIdentifierType, "i3") || plStringUtils::IsEqual((const char*)m_szIdentifierType, "int32"))
        {
          m_StateStack.PushBack(State::ReadingInt32);

          if (!m_bSkippingMode)
          {
            OnBeginPrimitiveList(plOpenDdlPrimitiveType::Int32, (const char*)m_szIdentifierName, bGlobalName);
          }
          return;
        }

        if (plStringUtils::IsEqual((const char*)m_szIdentifierType, "i1") || plStringUtils::IsEqual((const char*)m_szIdentifierType, "int8"))
        {
          m_StateStack.PushBack(State::ReadingInt8);

          if (!m_bSkippingMode)
          {
            OnBeginPrimitiveList(plOpenDdlPrimitiveType::Int8, (const char*)m_szIdentifierName, bGlobalName);
          }
          return;
        }

        if (plStringUtils::IsEqual((const char*)m_szIdentifierType, "i2") || plStringUtils::IsEqual((const char*)m_szIdentifierType, "int16"))
        {
          m_StateStack.PushBack(State::ReadingInt16);

          if (!m_bSkippingMode)
          {
            OnBeginPrimitiveList(plOpenDdlPrimitiveType::Int16, (const char*)m_szIdentifierName, bGlobalName);
          }
          return;
        }

        if (plStringUtils::IsEqual((const char*)m_szIdentifierType, "i4") || plStringUtils::IsEqual((const char*)m_szIdentifierType, "int64"))
        {
          m_StateStack.PushBack(State::ReadingInt64);

          if (!m_bSkippingMode)
          {
            OnBeginPrimitiveList(plOpenDdlPrimitiveType::Int64, (const char*)m_szIdentifierName, bGlobalName);
          }
          return;
        }
      }
      else
      {
        // support for f, d, s, b for  float, double, string, boo is an extension to OpenDDL

        if (plStringUtils::IsEqual((const char*)m_szIdentifierType, "f") || plStringUtils::IsEqual((const char*)m_szIdentifierType, "float"))
        {
          m_StateStack.PushBack(State::ReadingFloat);

          if (!m_bSkippingMode)
          {
            OnBeginPrimitiveList(plOpenDdlPrimitiveType::Float, (const char*)m_szIdentifierName, bGlobalName);
          }
          return;
        }

        if (plStringUtils::IsEqual((const char*)m_szIdentifierType, "s") || plStringUtils::IsEqual((const char*)m_szIdentifierType, "string"))
        {
          m_StateStack.PushBack(State::ReadingString);

          if (!m_bSkippingMode)
          {
            OnBeginPrimitiveList(plOpenDdlPrimitiveType::String, (const char*)m_szIdentifierName, bGlobalName);
          }
          return;
        }

        if (plStringUtils::IsEqual((const char*)m_szIdentifierType, "b") || plStringUtils::IsEqual((const char*)m_szIdentifierType, "bool"))
        {
          m_StateStack.PushBack(State::ReadingBool);

          if (!m_bSkippingMode)
          {
            OnBeginPrimitiveList(plOpenDdlPrimitiveType::Bool, (const char*)m_szIdentifierName, bGlobalName);
          }
          return;
        }

        if (plStringUtils::IsEqual((const char*)m_szIdentifierType, "d") || plStringUtils::IsEqual((const char*)m_szIdentifierType, "double"))
        {
          m_StateStack.PushBack(State::ReadingDouble);

          if (!m_bSkippingMode)
          {
            OnBeginPrimitiveList(plOpenDdlPrimitiveType::Double, (const char*)m_szIdentifierName, bGlobalName);
          }
          return;
        }
      }

      // else this is a custom object type
      {
        m_StateStack.PushBack(State::Idle);

        if (!m_bSkippingMode)
        {
          OnBeginObject((const char*)m_szIdentifierType, (const char*)m_szIdentifierName, bGlobalName);
        }
        return;
      }
    }
  }
}

void plOpenDdlParser::ReadIdentifier(plUInt8* szString, plUInt32& count)
{
  count = 0;

  if (m_uiCurByte == '\'')
  {
    /// \test This code path is unused so far

    // Extension to default OpenDDL: We allow identifier names to be surrounded with ' to contain any ASCII character

    if (!ReadCharacter())
    {
      ParsingError("Reached end of file while reading identifier", true);
      return;
    }

    while (m_uiCurByte != '\'' && count < s_uiMaxIdentifierLength)
    {
      szString[count] = m_uiCurByte;
      ++count;

      if (!ReadCharacter())
      {
        ParsingError("Reached end of file while reading identifier", true);
        return;
      }
    }

    if (count == s_uiMaxIdentifierLength)
    {
      szString[s_uiMaxIdentifierLength - 1] = '\0';

      ParsingError("Object type name is longer than 31 characters", false);

      // skip the rest
      while (m_uiCurByte != '\'')
      {
        if (!ReadCharacter())
        {
          ParsingError("Reached end of file while reading identifier", true);
          return;
        }
      }
    }
    else
    {
      szString[count] = '\0';
    }

    if (m_uiCurByte != '\'')
    {
      if (!ReadCharacter())
      {
        ParsingError("Reached end of file while reading identifier", true);
        return;
      }
    }
  }
  else
  {
    if (IsDdlIdentifierCharacter(m_uiCurByte))
    {
      szString[count] = m_uiCurByte;
      ++count;

      while ((IsDdlIdentifierCharacter(m_uiNextByte)) && count < s_uiMaxIdentifierLength)
      {
        if (!ReadCharacterSkipComments())
        {
          ParsingError("Reached end of file while reading identifier", true);
          return;
        }

        szString[count] = m_uiCurByte;
        ++count;
      }
    }

    if (count == s_uiMaxIdentifierLength)
    {
      szString[s_uiMaxIdentifierLength - 1] = '\0';

      ParsingError("Object type name is longer than 31 characters", false);

      // skip the rest
      while (IsDdlIdentifierCharacter(m_uiCurByte))
      {
        if (!ReadCharacterSkipComments())
        {
          ParsingError("Reached end of file while reading identifier", true);
          return;
        }
      }
    }
    else
    {
      szString[count] = '\0';
    }
  }

  SkipWhitespace();
}

void plOpenDdlParser::ReadString()
{
  m_uiTempStringLength = 0;

  while (true)
  {
    const bool bEscapeSequence = (m_uiCurByte == '\\');

    m_uiCurByte = '\0';

    if (!ReadCharacter())
    {
      ParsingError("While reading string: Reached end of document before end of string was found.", true);

      break; // stop when end of stream is encountered
    }

    if (!bEscapeSequence && m_uiCurByte == '\"')
      break;

    if (bEscapeSequence)
    {
      switch (m_uiCurByte)
      {
        case '\"':
          m_TempString[m_uiTempStringLength] = '\"';
          ++m_uiTempStringLength;
          break;
        case '\\':
          m_TempString[m_uiTempStringLength] = '\\';
          ++m_uiTempStringLength;
          m_uiCurByte = '\0'; // make sure the next character isn't interpreted as an escape sequence
          break;
        case '/':
          m_TempString[m_uiTempStringLength] = '/';
          ++m_uiTempStringLength;
          break;
        case 'b':
          m_TempString[m_uiTempStringLength] = '\b';
          ++m_uiTempStringLength;
          break;
        case 'f':
          m_TempString[m_uiTempStringLength] = '\f';
          ++m_uiTempStringLength;
          break;
        case 'n':
          m_TempString[m_uiTempStringLength] = '\n';
          ++m_uiTempStringLength;
          break;
        case 'r':
          m_TempString[m_uiTempStringLength] = '\r';
          ++m_uiTempStringLength;
          break;
        case 't':
          m_TempString[m_uiTempStringLength] = '\t';
          ++m_uiTempStringLength;
          break;
        case 'u':
          ParsingError("Unicode literals are not supported.", false);
          /// \todo Support escaped Unicode literals? (\u1234)
          break;
        default:
        {
          plStringBuilder s;
          s.SetFormat("Unknown escape-sequence '\\{0}'", plArgC(m_uiCurByte));
          ParsingError(s, false);
        }
        break;
      }
    }
    else if (m_uiCurByte != '\\')
    {
      m_TempString[m_uiTempStringLength] = m_uiCurByte;
      ++m_uiTempStringLength;
    }

    /// \todo Not sure if we can just use the other cache here, they might be used in parallel

    if (m_uiTempStringLength + 2 >= m_TempString.GetCount())
    {
      m_TempString.SetCountUninitialized(m_TempString.GetCount() * 2);
    }
  }

  m_TempString[m_uiTempStringLength] = '\0';
}

void plOpenDdlParser::ReadWord()
{
  m_uiTempStringLength = 0;

  do
  {
    m_TempString[m_uiTempStringLength] = m_uiCurByte;
    ++m_uiTempStringLength;

    m_uiCurByte = '\0';

    if (!ReadCharacterSkipComments())
      break; // stop when end of stream is encountered
  } while (!plStringUtils::IsIdentifierDelimiter_C_Code(m_uiCurByte));

  m_TempString[m_uiTempStringLength] = '\0';

  if (plStringUtils::IsWhiteSpace(m_uiCurByte))
    SkipWhitespace();
}

void plOpenDdlParser::PurgeCachedPrimitives(bool bThisIsAll)
{
  if (!m_bSkippingMode && m_uiNumCachedPrimitives > 0)
  {
    switch (m_StateStack.PeekBack().m_State)
    {
      case State::ReadingBool:
        OnPrimitiveBool(m_uiNumCachedPrimitives, m_pBoolCache, bThisIsAll);
        break;

      case State::ReadingInt8:
        OnPrimitiveInt8(m_uiNumCachedPrimitives, m_pInt8Cache, bThisIsAll);
        break;

      case State::ReadingInt16:
        OnPrimitiveInt16(m_uiNumCachedPrimitives, m_pInt16Cache, bThisIsAll);
        break;

      case State::ReadingInt32:
        OnPrimitiveInt32(m_uiNumCachedPrimitives, m_pInt32Cache, bThisIsAll);
        break;

      case State::ReadingInt64:
        OnPrimitiveInt64(m_uiNumCachedPrimitives, m_pInt64Cache, bThisIsAll);
        break;

      case State::ReadingUInt8:
        OnPrimitiveUInt8(m_uiNumCachedPrimitives, m_pUInt8Cache, bThisIsAll);
        break;

      case State::ReadingUInt16:
        OnPrimitiveUInt16(m_uiNumCachedPrimitives, m_pUInt16Cache, bThisIsAll);
        break;

      case State::ReadingUInt32:
        OnPrimitiveUInt32(m_uiNumCachedPrimitives, m_pUInt32Cache, bThisIsAll);
        break;

      case State::ReadingUInt64:
        OnPrimitiveUInt64(m_uiNumCachedPrimitives, m_pUInt64Cache, bThisIsAll);
        break;

      case State::ReadingFloat:
        OnPrimitiveFloat(m_uiNumCachedPrimitives, m_pFloatCache, bThisIsAll);
        break;

      case State::ReadingDouble:
        OnPrimitiveDouble(m_uiNumCachedPrimitives, m_pDoubleCache, bThisIsAll);
        break;

      default:
        PL_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }

  m_uiNumCachedPrimitives = 0;
}

bool plOpenDdlParser::ContinuePrimitiveList()
{
  switch (m_uiCurByte)
  {
    case '}':
    {
      PurgeCachedPrimitives(true);

      if (!m_bSkippingMode)
      {
        OnEndPrimitiveList();
      }

      SkipWhitespace();
      m_StateStack.PopBack();

      return false;
    }

    case ',':
    {
      // don't care about any number of semicolons
      /// \todo we could do an extra state 'expect , or }'

      SkipWhitespace();
      return false;
    }
  }

  return true;
}

void plOpenDdlParser::ContinueString()
{
  if (!ContinuePrimitiveList())
    return;

  switch (m_uiCurByte)
  {
    case '\"':
    {
      if (!m_bSkippingMode)
      {
        ReadString();
      }
      else
      {
        SkipString();
      }

      SkipWhitespace();

      if (!m_bSkippingMode)
      {
        plStringView view((const char*)&m_TempString[0], (const char*)&m_TempString[m_uiTempStringLength]);

        OnPrimitiveString(1, &view, false);
      }

      return;
    }

    default:
    {
      /// \todo better error message
      ParsingError("Expected , or } or a \"", true);
      return;
    }
  }
}

void plOpenDdlParser::SkipString()
{
  bool bEscapeSequence = false;

  do
  {
    bEscapeSequence = (m_uiCurByte == '\\');

    m_uiCurByte = '\0';

    if (!ReadCharacter())
    {
      ParsingError("While skipping string: Reached end of document before end of string was found.", true);

      return; // stop when end of stream is encountered
    }
  } while (bEscapeSequence || m_uiCurByte != '\"');
}

void plOpenDdlParser::ContinueBool()
{
  if (!ContinuePrimitiveList())
    return;

  switch (m_uiCurByte)
  {
    case '1':
    case '0':
    case 'f':
    case 't':
    {
      ReadWord();

      // Extension to OpenDDL: We allow everything that plConversionUtils::StringToBool knows as a bool value
      // We actually use '1' and '0' in compact mode

      bool bRes = false;
      if (plConversionUtils::StringToBool((const char*)&m_TempString[0], bRes) == PL_FAILURE)
      {
        plStringBuilder s;
        s.SetFormat("Parsing value: Expected 'true' or 'false', Got '{0}' instead.", (const char*)&m_TempString[0]);
        ParsingError(s.GetData(), false);
      }

      if (!m_bSkippingMode)
      {
        m_pBoolCache[m_uiNumCachedPrimitives++] = bRes;

        if (m_uiNumCachedPrimitives >= m_Cache.GetCount() / sizeof(bool))
          PurgeCachedPrimitives(false);
      }

      return;
    }
  }

  ParsingError("Invalid bool value", true);
}

void plOpenDdlParser::ContinueInt()
{
  if (!ContinuePrimitiveList())
    return;

  plInt8 sign = 1;

  // allow exactly one sign
  if (m_uiCurByte == '-')
    sign = -1;

  if (m_uiCurByte == '-' || m_uiCurByte == '+')
  {
    // no whitespace is allowed here
    ReadCharacterSkipComments();

    if (plStringUtils::IsWhiteSpace(m_uiCurByte))
    {
      ParsingError("Whitespace is not allowed between integer sign and value", false);
      SkipWhitespace();
    }
  }

  plUInt64 value = 0;

  if (m_uiCurByte == '0' && (m_uiNextByte == 'x' || m_uiNextByte == 'X'))
  {
    // HEX literal
    ParsingError("Integer HEX literals are not supported", true);
    return;
  }
  else if (m_uiCurByte == '0' && (m_uiNextByte == 'o' || m_uiNextByte == 'O'))
  {
    // Octal literal
    ParsingError("Integer Octal literals are not supported", true);
    return;
  }
  else if (m_uiCurByte == '0' && (m_uiNextByte == 'b' || m_uiNextByte == 'B'))
  {
    // Binary literal
    ParsingError("Integer Binary literals are not supported", true);
    return;
  }
  else if (m_uiCurByte == '\'')
  {
    // Character literal
    ParsingError("Integer Character literals are not supported", true);
    return;
  }
  else if (m_uiCurByte >= '0' && m_uiCurByte <= '9')
  {
    // Decimal literal
    value = ReadDecimalLiteral();
  }
  else
  {
    ParsingError("Malformed integer literal", true);
    return;
  }

  const auto curState = m_StateStack.PeekBack().m_State;
  if (curState >= State::ReadingUInt8 && curState <= State::ReadingUInt64)
  {
    if (sign < 0)
    {
      ParsingError("Cannot put a negative value into an unsigned integer type. Sign is ignored.", false);
    }
  }

  switch (curState)
  {
    case ReadingInt8:
    {
      m_pInt8Cache[m_uiNumCachedPrimitives++] = sign * (plInt8)value; // if user data is out of range, we don't care

      if (m_uiNumCachedPrimitives >= m_Cache.GetCount() / sizeof(plInt8))
        PurgeCachedPrimitives(false);

      break;
    }

    case ReadingInt16:
    {
      m_pInt16Cache[m_uiNumCachedPrimitives++] = sign * (plInt16)value; // if user data is out of range, we don't care

      if (m_uiNumCachedPrimitives >= m_Cache.GetCount() / sizeof(plInt16))
        PurgeCachedPrimitives(false);

      break;
    }

    case ReadingInt32:
    {
      m_pInt32Cache[m_uiNumCachedPrimitives++] = sign * (plInt32)value; // if user data is out of range, we don't care

      if (m_uiNumCachedPrimitives >= m_Cache.GetCount() / sizeof(plInt32))
        PurgeCachedPrimitives(false);

      break;
    }

    case ReadingInt64:
    {
      m_pInt64Cache[m_uiNumCachedPrimitives++] = sign * (plInt64)value; // if user data is out of range, we don't care

      if (m_uiNumCachedPrimitives >= m_Cache.GetCount() / sizeof(plInt64))
        PurgeCachedPrimitives(false);

      break;
    }


    case ReadingUInt8:
    {
      m_pUInt8Cache[m_uiNumCachedPrimitives++] = (plUInt8)value; // if user data is out of range, we don't care

      if (m_uiNumCachedPrimitives >= m_Cache.GetCount() / sizeof(plUInt8))
        PurgeCachedPrimitives(false);

      break;
    }

    case ReadingUInt16:
    {
      m_pUInt16Cache[m_uiNumCachedPrimitives++] = (plUInt16)value; // if user data is out of range, we don't care

      if (m_uiNumCachedPrimitives >= m_Cache.GetCount() / sizeof(plUInt16))
        PurgeCachedPrimitives(false);

      break;
    }

    case ReadingUInt32:
    {
      m_pUInt32Cache[m_uiNumCachedPrimitives++] = (plUInt32)value; // if user data is out of range, we don't care

      if (m_uiNumCachedPrimitives >= m_Cache.GetCount() / sizeof(plUInt32))
        PurgeCachedPrimitives(false);

      break;
    }

    case ReadingUInt64:
    {
      m_pUInt64Cache[m_uiNumCachedPrimitives++] = (plUInt64)value;

      if (m_uiNumCachedPrimitives >= m_Cache.GetCount() / sizeof(plUInt64))
        PurgeCachedPrimitives(false);

      break;
    }

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      break;
  }
}


void plOpenDdlParser::ContinueFloat()
{
  if (!ContinuePrimitiveList())
    return;

  const auto curState = m_StateStack.PeekBack().m_State;

  float sign = 1;

  // sign
  {
    if (m_uiCurByte == '-')
      sign = -1;

    if (m_uiCurByte == '-' || m_uiCurByte == '+')
    {
      // no whitespace is allowed here
      ReadCharacterSkipComments();

      if (plStringUtils::IsWhiteSpace(m_uiCurByte))
      {
        ParsingError("Whitespace is not allowed between float sign and value", false);
        SkipWhitespace();
      }
    }
  }

  double dValue = 0;
  float fValue = 0;

  if (m_uiCurByte == '0' && (m_uiNextByte == 'x' || m_uiNextByte == 'X'))
  {
    // skip the 0x part
    ReadCharacterSkipComments();
    ReadCharacterSkipComments();

    // HEX literal
    ReadHexString();

    if (curState == ReadingFloat)
      plConversionUtils::ConvertHexToBinary((const char*)m_TempString.GetData(), (plUInt8*)&fValue, 4);
    else
      plConversionUtils::ConvertHexToBinary((const char*)m_TempString.GetData(), (plUInt8*)&dValue, 8);
  }
  else if (m_uiCurByte == '0' && (m_uiNextByte == 'o' || m_uiNextByte == 'O'))
  {
    // Octal literal
    ParsingError("Float Octal literals are not supported", true);
    return;
  }
  else if (m_uiCurByte == '0' && (m_uiNextByte == 'b' || m_uiNextByte == 'B'))
  {
    // Binary literal
    ParsingError("Float Binary literals are not supported", true);
    return;
  }
  else if ((m_uiCurByte >= '0' && m_uiCurByte <= '9') || m_uiCurByte == '.')
  {
    // Decimal literal
    ReadDecimalFloat();

    if (plConversionUtils::StringToFloat((const char*)&m_TempString[0], dValue) == PL_FAILURE)
    {
      plStringBuilder s;
      s.SetFormat("Reading number failed: Could not convert '{0}' to a floating point value.", (const char*)&m_TempString[0]);
      ParsingError(s.GetData(), true);
    }

    fValue = (float)dValue;
  }
  else
  {
    ParsingError("Malformed float literal", true);
    return;
  }

  switch (curState)
  {
    case ReadingFloat:
    {
      m_pFloatCache[m_uiNumCachedPrimitives++] = sign * fValue;

      if (m_uiNumCachedPrimitives >= m_Cache.GetCount() / sizeof(float))
        PurgeCachedPrimitives(false);

      break;
    }

    case ReadingDouble:
    {
      m_pDoubleCache[m_uiNumCachedPrimitives++] = sign * dValue;

      if (m_uiNumCachedPrimitives >= m_Cache.GetCount() / sizeof(double))
        PurgeCachedPrimitives(false);

      break;
    }

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      break;
  }
}

void plOpenDdlParser::ReadDecimalFloat()
{
  m_uiTempStringLength = 0;

  do
  {
    m_TempString[m_uiTempStringLength] = m_uiCurByte;
    ++m_uiTempStringLength;

    m_uiCurByte = '\0';

    if (!ReadCharacterSkipComments())
      break; // stop when end of stream is encountered
  } while ((m_uiCurByte >= '0' && m_uiCurByte <= '9') || m_uiCurByte == '.' || m_uiCurByte == 'e' || m_uiCurByte == 'E' || m_uiCurByte == '-' ||
           m_uiCurByte == '+' || m_uiCurByte == '_');

  m_TempString[m_uiTempStringLength] = '\0';

  if (plStringUtils::IsWhiteSpace(m_uiCurByte))
    SkipWhitespace();
}


void plOpenDdlParser::ReadHexString()
{
  m_uiTempStringLength = 0;

  do
  {
    m_TempString[m_uiTempStringLength] = m_uiCurByte;
    ++m_uiTempStringLength;

    m_uiCurByte = '\0';

    if (!ReadCharacterSkipComments())
      break; // stop when end of stream is encountered
  } while ((m_uiCurByte >= '0' && m_uiCurByte <= '9') || (m_uiCurByte >= 'a' && m_uiCurByte <= 'f') || (m_uiCurByte >= 'A' && m_uiCurByte <= 'F'));

  m_TempString[m_uiTempStringLength] = '\0';

  if (plStringUtils::IsWhiteSpace(m_uiCurByte))
    SkipWhitespace();
}

plUInt64 plOpenDdlParser::ReadDecimalLiteral()
{
  plUInt64 value = 0;

  while ((m_uiCurByte >= '0' && m_uiCurByte <= '9') || m_uiCurByte == '_')
  {
    if (m_uiCurByte == '_')
    {
      ReadCharacterSkipComments();
      continue;
    }

    // won't check for overflow
    value *= 10;
    value += (m_uiCurByte - '0');

    ReadCharacterSkipComments();
  }

  if (plStringUtils::IsWhiteSpace(m_uiCurByte))
  {
    // move to next valid character
    SkipWhitespace();
  }

  return value;
}


