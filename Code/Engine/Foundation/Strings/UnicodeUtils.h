#pragma once

#include <Foundation/Basics.h>
#include <Foundation/ThirdParty/utf8/utf8.h>



/// \brief Helper functions to work with Unicode.
class PL_FOUNDATION_DLL plUnicodeUtils
{
public:
  /// \brief [internal] Returns the max string end pointer for the given type
  template <typename T>
  static constexpr T* GetMaxStringEnd();

public:
  /// \brief Byte Order Mark for Little Endian Utf16 strings.
  static constexpr plUInt16 Utf16BomLE = 0xfeff;

  /// \brief Byte Order Mark for Big Endian Utf16 strings.
  static constexpr plUInt16 Utf16BomBE = 0xfffe;

  /// \brief Returns whether a character is a pure ASCII character (only the first 7 Bits are used)
  static bool IsASCII(plUInt32 uiChar); // [tested]

  /// \brief Checks whether the given byte is a start byte in a UTF-8 multi-byte sequence
  static bool IsUtf8StartByte(char iByte); // [tested]

  /// \brief Checks whether the given byte is a byte in a UTF-8 multi-byte sequence.
  static bool IsUtf8ContinuationByte(char iByte); // [tested]

  /// \brief Returns the number of bytes that a UTF-8 sequence is in length, which is encoded in the first byte of the sequence.
  static plUInt32 GetUtf8SequenceLength(char iFirstByte); // [tested]

  /// \brief Converts the UTF-8 character that starts at pFirstChar into a UTF-32 character.
  static plUInt32 ConvertUtf8ToUtf32(const char* pFirstChar); // [tested]

  /// \brief Computes how many bytes the character would require, if encoded in UTF-8.
  static plUInt32 GetSizeForCharacterInUtf8(plUInt32 uiCharacter); // [tested]

  /// \brief Moves the given string pointer ahead to the next Utf8 character sequence.
  ///
  /// The string may point to an invalid position (in between a character sequence).
  /// It may not point to a zero terminator already.
  static plResult MoveToNextUtf8(const char*& ref_szUtf8, plUInt32 uiNumCharacters = 1); // [tested]

  /// \brief Moves the given string pointer ahead to the next Utf8 character sequence.
  ///
  /// The string may point to an invalid position (in between a character sequence).
  /// It may not point to a zero terminator already.
  static plResult MoveToNextUtf8(const char*& ref_szUtf8, const char* szUtf8End, plUInt32 uiNumCharacters = 1); // [tested]

  /// \brief Moves the given string pointer backwards to the previous Utf8 character sequence.
  ///
  /// The string may point to an invalid position (in between a character sequence), or even the \0 terminator,
  /// as long as there is a valid string before it (and the user knows when to stop).
  static plResult MoveToPriorUtf8(const char*& ref_szUtf8, const char* szUtf8Start, plUInt32 uiNumCharacters = 1); // [tested]

  /// \brief Returns false if the given string does not contain a completely valid Utf8 string.
  static bool IsValidUtf8(const char* szString, const char* szStringEnd = GetMaxStringEnd<char>());

  /// \brief If the given string starts with a Utf8 Bom, the pointer is incremented behind the Bom, and the function returns true.
  ///
  /// Otherwise the pointer is unchanged and false is returned.
  static bool SkipUtf8Bom(const char*& ref_szUtf8); // [tested]

  /// \brief If the given string starts with a Utf16 little endian Bom, the pointer is incremented behind the Bom, and the function returns
  /// true.
  ///
  /// Otherwise the pointer is unchanged and false is returned.
  static bool SkipUtf16BomLE(const plUInt16*& ref_pUtf16); // [tested]

  /// \brief If the given string starts with a Utf16 big endian Bom, the pointer is incremented behind the Bom, and the function returns
  /// true.
  ///
  /// Otherwise the pointer is unchanged and false is returned.
  static bool SkipUtf16BomBE(const plUInt16*& ref_pUtf16); // [tested]

  /// \brief Decodes the next character from the given Utf8 sequence to Utf32 and increments the iterator as far as necessary.
  template <typename ByteIterator>
  static plUInt32 DecodeUtf8ToUtf32(ByteIterator& ref_szUtf8Iterator); // [tested]

  /// \brief Characters that cannot be represented in a single utf16 code point need to be split up into two surrogate pairs to form unicode characters beyond the \uFFFF range.
  template <typename UInt16Iterator>
  static bool IsUtf16Surrogate(UInt16Iterator& ref_szUtf16Iterator); // [tested]

  /// \brief Decodes the next character from the given Utf16 sequence to Utf32 and increments the iterator as far as necessary.
  template <typename UInt16Iterator>
  static plUInt32 DecodeUtf16ToUtf32(UInt16Iterator& ref_szUtf16Iterator); // [tested]

  /// \brief Decodes the next character from the given wchar_t sequence to Utf32 and increments the iterator as far as necessary.
  template <typename WCharIterator>
  static plUInt32 DecodeWCharToUtf32(WCharIterator& ref_szWCharIterator); // [tested]

  /// \brief Encodes the given Utf32 character to Utf8 and writes as many bytes to the output iterator, as necessary.
  template <typename ByteIterator>
  static void EncodeUtf32ToUtf8(plUInt32 uiUtf32, ByteIterator& ref_szUtf8Output); // [tested]

  /// \brief Encodes the given Utf32 character to Utf16 and writes as many bytes to the output iterator, as necessary.
  template <typename UInt16Iterator>
  static void EncodeUtf32ToUtf16(plUInt32 uiUtf32, UInt16Iterator& ref_szUtf16Output); // [tested]

  /// \brief Encodes the given Utf32 character to wchar_t and writes as many bytes to the output iterator, as necessary.
  template <typename WCharIterator>
  static void EncodeUtf32ToWChar(plUInt32 uiUtf32, WCharIterator& ref_szWCharOutput); // [tested]

  /// \brief [internal] Small helper class to append bytes to some arbitrary container. Used for Utf8 string building.
  template <typename IntType, typename Container>
  struct UtfInserter
  {
    using InsertionType = IntType;

    PL_ALWAYS_INLINE UtfInserter(Container* pContainer) { m_pContainer = pContainer; }
    PL_ALWAYS_INLINE void operator++() {}
    PL_ALWAYS_INLINE UtfInserter& operator++(int) { return *this; }
    PL_ALWAYS_INLINE void operator=(IntType rhs) { m_pContainer->PushBack(rhs); }
    PL_ALWAYS_INLINE UtfInserter& operator*() { return *this; }

    Container* m_pContainer;
  };

  /// \brief Checks an array of char's, whether it is a valid Utf8 string. If not, it repairs the string, ie by either re-encoding characters or removing them.
  /// Writes the result to the desired container type (plString or plStringBuilder).
  ///
  /// Returns true if the text had to be repaired, false if it was already valid.
  ///
  /// \note That the for #include order reasons, the implementation is in StringBuilder_inl.h, so you need to have StringBuilder.h included to use it.
  template <typename Container>
  static bool RepairNonUtf8Text(const char* pStartData, const char* pEndData, Container& out_result);
};

#include <Foundation/Strings/Implementation/UnicodeUtils_inl.h>
