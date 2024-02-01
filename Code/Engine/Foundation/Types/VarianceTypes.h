#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>
#include <Foundation/Types/TypeTraits.h>

#define PL_DECLARE_VARIANCE_HASH_HELPER(TYPE)                        \
  template <>                                                        \
  struct plHashHelper<TYPE>                                          \
  {                                                                  \
    PL_ALWAYS_INLINE static plUInt32 Hash(const TYPE& value)         \
    {                                                                \
      return plHashingUtils::xxHash32(&value, sizeof(TYPE));         \
    }                                                                \
    PL_ALWAYS_INLINE static bool Equal(const TYPE& a, const TYPE& b) \
    {                                                                \
      return a == b;                                                 \
    }                                                                \
  };

struct PL_FOUNDATION_DLL plVarianceTypeBase
{
  PL_DECLARE_POD_TYPE();

  float m_fVariance = 0;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plVarianceTypeBase);

struct PL_FOUNDATION_DLL plVarianceTypeFloat : public plVarianceTypeBase
{
  PL_DECLARE_POD_TYPE();
  bool operator==(const plVarianceTypeFloat& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const plVarianceTypeFloat& rhs) const
  {
    return !(*this == rhs);
  }
  float m_Value = 0;
};

PL_DECLARE_VARIANCE_HASH_HELPER(plVarianceTypeFloat);
PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plVarianceTypeFloat);
PL_DECLARE_CUSTOM_VARIANT_TYPE(plVarianceTypeFloat);

struct PL_FOUNDATION_DLL plVarianceTypeTime : public plVarianceTypeBase
{
  PL_DECLARE_POD_TYPE();
  bool operator==(const plVarianceTypeTime& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const plVarianceTypeTime& rhs) const
  {
    return !(*this == rhs);
  }
  plTime m_Value;
};

PL_DECLARE_VARIANCE_HASH_HELPER(plVarianceTypeTime);
PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plVarianceTypeTime);
PL_DECLARE_CUSTOM_VARIANT_TYPE(plVarianceTypeTime);

struct PL_FOUNDATION_DLL plVarianceTypeAngle : public plVarianceTypeBase
{
  PL_DECLARE_POD_TYPE();
  bool operator==(const plVarianceTypeAngle& rhs) const
  {
    return m_fVariance == rhs.m_fVariance && m_Value == rhs.m_Value;
  }
  bool operator!=(const plVarianceTypeAngle& rhs) const
  {
    return !(*this == rhs);
  }
  plAngle m_Value;
};

PL_DECLARE_VARIANCE_HASH_HELPER(plVarianceTypeAngle);
PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plVarianceTypeAngle);
PL_DECLARE_CUSTOM_VARIANT_TYPE(plVarianceTypeAngle);
