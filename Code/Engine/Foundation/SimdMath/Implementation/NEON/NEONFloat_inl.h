#pragma once

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat()
{
  PL_CHECK_SIMD_ALIGNMENT(this);

#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  // Initialize all data to NaN in debug mode to find problems with uninitialized data easier.
  m_v = vmovq_n_f32(plMath::NaN<float>());
#endif
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(float f)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_f32(f);
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(plInt32 i)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = vcvtq_f32_s32(vmovq_n_s32(i));
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(plUInt32 i)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = vcvtq_f32_u32(vmovq_n_u32(i));
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(plAngle a)
{
  PL_CHECK_SIMD_ALIGNMENT(this);

  m_v = vmovq_n_f32(a.GetRadian());
}

PL_ALWAYS_INLINE plSimdFloat::plSimdFloat(plInternal::QuadFloat v)
{
  m_v = v;
}

PL_ALWAYS_INLINE plSimdFloat::operator float() const
{
  return vgetq_lane_f32(m_v, 0);
}

// static
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::MakeZero()
{
  return vmovq_n_f32(0.0f);
}

// static
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::MakeNaN()
{
  return vmovq_n_f32(plMath::NaN<float>());
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::operator+(const plSimdFloat& f) const
{
  return vaddq_f32(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::operator-(const plSimdFloat& f) const
{
  return vsubq_f32(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::operator*(const plSimdFloat& f) const
{
  return vmulq_f32(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::operator/(const plSimdFloat& f) const
{
  return vdivq_f32(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat& plSimdFloat::operator+=(const plSimdFloat& f)
{
  m_v = vaddq_f32(m_v, f.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdFloat& plSimdFloat::operator-=(const plSimdFloat& f)
{
  m_v = vsubq_f32(m_v, f.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdFloat& plSimdFloat::operator*=(const plSimdFloat& f)
{
  m_v = vmulq_f32(m_v, f.m_v);
  return *this;
}

PL_ALWAYS_INLINE plSimdFloat& plSimdFloat::operator/=(const plSimdFloat& f)
{
  m_v = vdivq_f32(m_v, f.m_v);
  return *this;
}

PL_ALWAYS_INLINE bool plSimdFloat::IsEqual(const plSimdFloat& rhs, const plSimdFloat& fEpsilon) const
{
  plSimdFloat minusEps = rhs - fEpsilon;
  plSimdFloat plusEps = rhs + fEpsilon;
  return ((*this >= minusEps) && (*this <= plusEps));
}

PL_ALWAYS_INLINE bool plSimdFloat::operator==(const plSimdFloat& f) const
{
  return vgetq_lane_u32(vceqq_f32(m_v, f.m_v), 0) & 1;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator!=(const plSimdFloat& f) const
{
  return !operator==(f);
}

PL_ALWAYS_INLINE bool plSimdFloat::operator>=(const plSimdFloat& f) const
{
  return vgetq_lane_u32(vcgeq_f32(m_v, f.m_v), 0) & 1;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator>(const plSimdFloat& f) const
{
  return vgetq_lane_u32(vcgtq_f32(m_v, f.m_v), 0) & 1;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator<=(const plSimdFloat& f) const
{
  return vgetq_lane_u32(vcleq_f32(m_v, f.m_v), 0) & 1;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator<(const plSimdFloat& f) const
{
  return vgetq_lane_u32(vcltq_f32(m_v, f.m_v), 0) & 1;
}

PL_ALWAYS_INLINE bool plSimdFloat::operator==(float f) const
{
  return (*this) == plSimdFloat(f);
}

PL_ALWAYS_INLINE bool plSimdFloat::operator!=(float f) const
{
  return (*this) != plSimdFloat(f);
}

PL_ALWAYS_INLINE bool plSimdFloat::operator>(float f) const
{
  return (*this) > plSimdFloat(f);
}

PL_ALWAYS_INLINE bool plSimdFloat::operator>=(float f) const
{
  return (*this) >= plSimdFloat(f);
}

PL_ALWAYS_INLINE bool plSimdFloat::operator<(float f) const
{
  return (*this) < plSimdFloat(f);
}

PL_ALWAYS_INLINE bool plSimdFloat::operator<=(float f) const
{
  return (*this) <= plSimdFloat(f);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetReciprocal<plMathAcc::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), m_v);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetReciprocal<plMathAcc::BITS_23>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // Two iterations of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);
  float32x4_t x2 = vmulq_f32(vrecpsq_f32(m_v, x1), x1);

  return x2;
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetReciprocal<plMathAcc::BITS_12>() const
{
  float32x4_t x0 = vrecpeq_f32(m_v);

  // One iteration of Newton-Raphson
  float32x4_t x1 = vmulq_f32(vrecpsq_f32(m_v, x0), x0);

  return x1;
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetInvSqrt<plMathAcc::FULL>() const
{
  return vdivq_f32(vmovq_n_f32(1.0f), vsqrtq_f32(m_v));
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetInvSqrt<plMathAcc::BITS_23>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // Two iterations of Newton-Raphson
  const float32x4_t x1 = vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x1, m_v), x1), x1);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetInvSqrt<plMathAcc::BITS_12>() const
{
  const float32x4_t x0 = vrsqrteq_f32(m_v);

  // One iteration of Newton-Raphson
  return vmulq_f32(vrsqrtsq_f32(vmulq_f32(x0, m_v), x0), x0);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetSqrt<plMathAcc::FULL>() const
{
  return vsqrtq_f32(m_v);
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetSqrt<plMathAcc::BITS_23>() const
{
  return (*this) * GetInvSqrt<plMathAcc::BITS_23>();
}

template <>
PL_ALWAYS_INLINE plSimdFloat plSimdFloat::GetSqrt<plMathAcc::BITS_12>() const
{
  return (*this) * GetInvSqrt<plMathAcc::BITS_12>();
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::Max(const plSimdFloat& f) const
{
  return vmaxq_f32(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::Min(const plSimdFloat& f) const
{
  return vminq_f32(m_v, f.m_v);
}

PL_ALWAYS_INLINE plSimdFloat plSimdFloat::Abs() const
{
  return vabsq_f32(m_v);
}
