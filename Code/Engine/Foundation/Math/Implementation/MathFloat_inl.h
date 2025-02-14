#pragma once

#include <algorithm>

namespace plMath
{
  PL_ALWAYS_INLINE bool IsFinite(float value)
  {
    // Check the 8 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    plIntFloatUnion i2f(value);
    return ((i2f.i & 0x7f800000u) != 0x7f800000u);
  }

  PL_ALWAYS_INLINE bool IsNaN(float value)
  {
    // Check the 8 exponent bits.
    // NAN -> (exponent = all 1, mantissa = non-zero)
    // INF -> (exponent = all 1, mantissa = zero)

    plIntFloatUnion i2f(value);
    return (((i2f.i & 0x7f800000u) == 0x7f800000u) && ((i2f.i & 0x7FFFFFu) != 0));
  }

  PL_ALWAYS_INLINE float Floor(float f) { return floorf(f); }

  PL_ALWAYS_INLINE float Ceil(float f) { return ceilf(f); }

  PL_ALWAYS_INLINE float Round(float f) { return Floor(f + 0.5f); }

  PL_ALWAYS_INLINE float RoundToMultiple(float f, float fMultiple) { return Round(f / fMultiple) * fMultiple; }


  inline float RoundDown(float f, float fMultiple)
  {
    float fDivides = f / fMultiple;
    float fFactor = Floor(fDivides);
    return fFactor * fMultiple;
  }

  inline float RoundUp(float f, float fMultiple)
  {
    float fDivides = f / fMultiple;
    float fFactor = Ceil(fDivides);
    return fFactor * fMultiple;
  }

  PL_ALWAYS_INLINE float Sin(plAngle a) { return sinf(a.GetRadian()); }

  PL_ALWAYS_INLINE float Cos(plAngle a) { return cosf(a.GetRadian()); }

  PL_ALWAYS_INLINE float Tan(plAngle a) { return tanf(a.GetRadian()); }

  PL_ALWAYS_INLINE plAngle ASin(float f) { return plAngle::MakeFromRadian(asinf(f)); }

  PL_ALWAYS_INLINE plAngle ACos(float f) { return plAngle::MakeFromRadian(acosf(f)); }

  PL_ALWAYS_INLINE plAngle ATan(float f) { return plAngle::MakeFromRadian(atanf(f)); }

  PL_ALWAYS_INLINE plAngle ATan2(float y, float x) { return plAngle::MakeFromRadian(atan2f(y, x)); }

  PL_ALWAYS_INLINE float Exp(float f) { return expf(f); }

  PL_ALWAYS_INLINE float Ln(float f) { return logf(f); }

  PL_ALWAYS_INLINE float Log2(float f) { return log2f(f); }

  PL_ALWAYS_INLINE float Log10(float f) { return log10f(f); }

  PL_ALWAYS_INLINE float Log(float fBase, float f) { return log10f(f) / log10f(fBase); }

  PL_ALWAYS_INLINE float Pow2(float f) { return exp2f(f); }

  PL_ALWAYS_INLINE float Pow(float fBase, float fExp) { return powf(fBase, fExp); }

  PL_ALWAYS_INLINE float Root(float f, float fNthRoot) { return powf(f, 1.0f / fNthRoot); }

  PL_ALWAYS_INLINE float Sqrt(float f) { return sqrtf(f); }

  PL_ALWAYS_INLINE float Mod(float f, float fDiv) { return fmodf(f, fDiv); }
} // namespace plMath
