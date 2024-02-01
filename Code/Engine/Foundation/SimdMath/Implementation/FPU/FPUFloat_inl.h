#pragma once

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat() {}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(float f)
{
  m_v.Set(f);
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(plInt32 i)
{
  m_v.Set((float)i);
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(plUInt32 i)
{
  m_v.Set((float)i);
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(plAngle a)
{
  m_v.Set(a.GetRadian());
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(plInternal::QuadFloat v)
{
  m_v = v;
}

PL_ALWAYS_INLINE plSimdFloat::operator float() const
{
  return m_v.x;
}

// static
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::MakeZero()
{
  return plSimdFloat(0.0f);
}

// static
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::MakeNaN()
{
  return plSimdFloat(plMath::NaN<float>());
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::operator+(const plSimdFloat& f) const
{
  return m_v + f.m_v;
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::operator-(const plSimdFloat& f) const
{
  return m_v - f.m_v;
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::operator*(const plSimdFloat& f) const
{
  return m_v.CompMul(f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::operator/(const plSimdFloat& f) const
{
  return m_v.CompDiv(f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat& plSimdFloat::operator+=(const plSimdFloat& f)
{
  m_v += f.m_v;
  return *this;
}

PL_ALWAYS_INLINE plSimdFloat& plSimdFloat::operator-=(const plSimdFloat& f)
{
  m_v -= f.m_v;
  return *this;
}

PL_ALWAYS_INLINE plSimdFloat& plSimdFloat::operator*=(const plSimdFloat& f)
{
  m_v = m_v.CompMul(f.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdFloat& plSimdFloat::operator/=(const plSimdFloat& f)
{
  m_v = m_v.CompDiv(f.m_v);
  return *this;
}

PL_ALWAYS_INLINE bool plSimdFloat::IsEqual(const plSimdFloat& rhs, const plSimdFloat& fEpsilon) const
{
  return m_v.IsEqual(rhs.m_v, fEpsilon);
}

PL_ALWAYS_INLINE bool plSimdFloat::operator==(const plSimdFloat& f) const
{
  return m_v.x == f.m_v.x;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator!=(const plSimdFloat& f) const
{
  return m_v.x != f.m_v.x;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator>=(const plSimdFloat& f) const
{
  return m_v.x >= f.m_v.x;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator>(const plSimdFloat& f) const
{
  return m_v.x > f.m_v.x;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator<=(const plSimdFloat& f) const
{
  return m_v.x <= f.m_v.x;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator<(const plSimdFloat& f) const
{
  return m_v.x < f.m_v.x;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator==(float f) const
{
  return m_v.x == f;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator!=(float f) const
{
  return m_v.x != f;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator>(float f) const
{
  return m_v.x > f;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator>=(float f) const
{
  return m_v.x >= f;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator<(float f) const
{
  return m_v.x < f;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator<=(float f) const
{
  return m_v.x <= f;
}

template <plMathAcc::Enum acc>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetReciprocal() const
{
  return plSimdFloat(1.0f / m_v.x);
}

template <plMathAcc::Enum acc>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetSqrt() const
{
  return plSimdFloat(plMath::Sqrt(m_v.x));
}

template <plMathAcc::Enum acc>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetInvSqrt() const
{
  return plSimdFloat(1.0f / plMath::Sqrt(m_v.x));
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::Max(const plSimdFloat& f) const
{
  return m_v.CompMax(f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::Min(const plSimdFloat& f) const
{
  return m_v.CompMin(f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::Abs() const
{
  return plSimdFloat(plMath::Abs(m_v.x));
}
