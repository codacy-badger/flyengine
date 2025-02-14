#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/HashedString.h>

/// \brief A single stream in a stream group holding contiguous data of a given type.
class PL_FOUNDATION_DLL plProcessingStream
{
public:
  /// \brief The data types which can be stored in the stream.
  /// When adding new data types the GetDataTypeSize() of plProcessingStream needs to be updated.
  enum class DataType : plUInt8
  {
    Half,  // plFloat16
    Half2, // 2x plFloat16
    Half3, // 3x plFloat16
    Half4, // 4x plFloat16

    Float,  // float
    Float2, // 2x float, e.g. plVec2
    Float3, // 3x float, e.g. plVec3
    Float4, // 4x float, e.g. plVec4

    Byte,
    Byte2,
    Byte3,
    Byte4,

    Short,
    Short2,
    Short3,
    Short4,

    Int,
    Int2,
    Int3,
    Int4,

    Count
  };

  plProcessingStream();
  plProcessingStream(const plHashedString& sName, DataType type, plUInt16 uiStride, plUInt16 uiAlignment);
  plProcessingStream(const plHashedString& sName, plArrayPtr<plUInt8> data, DataType type, plUInt16 uiStride);
  plProcessingStream(const plHashedString& sName, plArrayPtr<plUInt8> data, DataType type);
  ~plProcessingStream();

  /// \brief Returns a const pointer to the data casted to the type T, note that no type check is done!
  template <typename T>
  const T* GetData() const
  {
    return static_cast<const T*>(GetData());
  }

  /// \brief Returns a const pointer to the start of the data block.
  const void* GetData() const { return m_pData; }

  /// \brief Returns a non-const pointer to the data casted to the type T, note that no type check is done!
  template <typename T>
  T* GetWritableData() const
  {
    return static_cast<T*>(GetWritableData());
  }

  /// \brief Returns a non-const pointer to the start of the data block.
  void* GetWritableData() const { return m_pData; }

  plUInt64 GetDataSize() const { return m_uiDataSize; }

  /// \brief Returns the name of the stream
  const plHashedString& GetName() const { return m_sName; }

  /// \brief Returns the alignment which was used to allocate the stream.
  plUInt16 GetAlignment() const { return m_uiAlignment; }

  /// \brief Returns the data type of the stream.
  DataType GetDataType() const { return m_Type; }

  /// \brief Returns the size of one stream element in bytes.
  plUInt16 GetElementSize() const { return m_uiTypeSize; }

  /// \brief Returns the stride between two elements of the stream in bytes.
  plUInt16 GetElementStride() const { return m_uiStride; }

  static plUInt16 GetDataTypeSize(DataType type);
  static plStringView GetDataTypeName(DataType type);

protected:
  friend class plProcessingStreamGroup;

  void SetSize(plUInt64 uiNumElements);
  void FreeData();

  void* m_pData = nullptr;
  plUInt64 m_uiDataSize = 0; // in bytes

  plUInt16 m_uiAlignment = 0;
  plUInt16 m_uiTypeSize = 0;
  plUInt16 m_uiStride = 0;
  DataType m_Type;
  bool m_bExternalMemory = false;

  plHashedString m_sName;
};
