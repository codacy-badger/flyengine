#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/Tracks/ColorGradient.h>

plColorGradient::plColorGradient()
{
  Clear();
}


void plColorGradient::Clear()
{
  m_ColorCPs.Clear();
  m_AlphaCPs.Clear();
  m_IntensityCPs.Clear();
}


bool plColorGradient::IsEmpty() const
{
  return m_ColorCPs.IsEmpty() && m_AlphaCPs.IsEmpty() && m_IntensityCPs.IsEmpty();
}

void plColorGradient::AddColorControlPoint(double x, const plColorGammaUB& rgb)
{
  auto& cp = m_ColorCPs.ExpandAndGetRef();
  cp.m_PosX = x;
  cp.m_GammaRed = rgb.r;
  cp.m_GammaGreen = rgb.g;
  cp.m_GammaBlue = rgb.b;
}

void plColorGradient::AddAlphaControlPoint(double x, plUInt8 uiAlpha)
{
  auto& cp = m_AlphaCPs.ExpandAndGetRef();
  cp.m_PosX = x;
  cp.m_Alpha = uiAlpha;
}

void plColorGradient::AddIntensityControlPoint(double x, float fIntensity)
{
  auto& cp = m_IntensityCPs.ExpandAndGetRef();
  cp.m_PosX = x;
  cp.m_Intensity = fIntensity;
}

bool plColorGradient::GetExtents(double& ref_fMinx, double& ref_fMaxx) const
{
  ref_fMinx = plMath::MaxValue<float>();
  ref_fMaxx = -plMath::MaxValue<float>();

  for (const auto& cp : m_ColorCPs)
  {
    ref_fMinx = plMath::Min(ref_fMinx, cp.m_PosX);
    ref_fMaxx = plMath::Max(ref_fMaxx, cp.m_PosX);
  }

  for (const auto& cp : m_AlphaCPs)
  {
    ref_fMinx = plMath::Min(ref_fMinx, cp.m_PosX);
    ref_fMaxx = plMath::Max(ref_fMaxx, cp.m_PosX);
  }

  for (const auto& cp : m_IntensityCPs)
  {
    ref_fMinx = plMath::Min(ref_fMinx, cp.m_PosX);
    ref_fMaxx = plMath::Max(ref_fMaxx, cp.m_PosX);
  }

  return ref_fMinx <= ref_fMaxx;
}

void plColorGradient::GetNumControlPoints(plUInt32& ref_uiRgb, plUInt32& ref_uiAlpha, plUInt32& ref_uiIntensity) const
{
  ref_uiRgb = m_ColorCPs.GetCount();
  ref_uiAlpha = m_AlphaCPs.GetCount();
  ref_uiIntensity = m_IntensityCPs.GetCount();
}

void plColorGradient::SortControlPoints()
{
  m_ColorCPs.Sort();
  m_AlphaCPs.Sort();
  m_IntensityCPs.Sort();

  PrecomputeLerpNormalizer();
}

void plColorGradient::PrecomputeLerpNormalizer()
{
  for (plUInt32 i = 1; i < m_ColorCPs.GetCount(); ++i)
  {
    const double px0 = m_ColorCPs[i - 1].m_PosX;
    const double px1 = m_ColorCPs[i].m_PosX;

    const double dist = px1 - px0;
    const double invDist = 1.0 / dist;

    m_ColorCPs[i - 1].m_fInvDistToNextCp = (float)invDist;
  }

  for (plUInt32 i = 1; i < m_AlphaCPs.GetCount(); ++i)
  {
    const double px0 = m_AlphaCPs[i - 1].m_PosX;
    const double px1 = m_AlphaCPs[i].m_PosX;

    const double dist = px1 - px0;
    const double invDist = 1.0 / dist;

    m_AlphaCPs[i - 1].m_fInvDistToNextCp = (float)invDist;
  }

  for (plUInt32 i = 1; i < m_IntensityCPs.GetCount(); ++i)
  {
    const double px0 = m_IntensityCPs[i - 1].m_PosX;
    const double px1 = m_IntensityCPs[i].m_PosX;

    const double dist = px1 - px0;
    const double invDist = 1.0 / dist;

    m_IntensityCPs[i - 1].m_fInvDistToNextCp = (float)invDist;
  }
}

void plColorGradient::Evaluate(double x, plColorGammaUB& ref_rgba, float& ref_fIntensity) const
{
  ref_rgba.r = 255;
  ref_rgba.g = 255;
  ref_rgba.b = 255;
  ref_rgba.a = 255;
  ref_fIntensity = 1.0f;

  EvaluateColor(x, ref_rgba);
  EvaluateAlpha(x, ref_rgba.a);
  EvaluateIntensity(x, ref_fIntensity);
}


void plColorGradient::Evaluate(double x, plColor& ref_hdr) const
{
  float intensity = 1.0f;
  plUInt8 alpha = 255;

  EvaluateColor(x, ref_hdr);
  EvaluateAlpha(x, alpha);
  EvaluateIntensity(x, intensity);

  ref_hdr.ScaleRGB(intensity);
  ref_hdr.a = plMath::ColorByteToFloat(alpha);
}

void plColorGradient::EvaluateColor(double x, plColorGammaUB& ref_rgb) const
{
  plColor hdr;
  EvaluateColor(x, hdr);

  ref_rgb = hdr;
  ref_rgb.a = 255;
}

void plColorGradient::EvaluateColor(double x, plColor& ref_rgb) const
{
  ref_rgb.r = 1.0f;
  ref_rgb.g = 1.0f;
  ref_rgb.b = 1.0f;
  ref_rgb.a = 1.0f;

  const plUInt32 numCPs = m_ColorCPs.GetCount();

  if (numCPs >= 2)
  {
    // clamp to left value
    if (m_ColorCPs[0].m_PosX >= x)
    {
      const ColorCP& cp = m_ColorCPs[0];
      ref_rgb = plColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
      return;
    }

    plUInt32 uiControlPoint;

    for (plUInt32 i = 1; i < numCPs; ++i)
    {
      if (m_ColorCPs[i].m_PosX >= x)
      {
        uiControlPoint = i - 1;
        goto found;
      }
    }

    // no point found -> clamp to right value
    {
      const ColorCP& cp = m_ColorCPs[numCPs - 1];
      ref_rgb = plColorGammaUB(cp.m_GammaRed, cp.m_GammaGreen, cp.m_GammaBlue);
      return;
    }

  found:
  {
    const ColorCP& cpl = m_ColorCPs[uiControlPoint];
    const ColorCP& cpr = m_ColorCPs[uiControlPoint + 1];

    const plColor lhs(plColorGammaUB(cpl.m_GammaRed, cpl.m_GammaGreen, cpl.m_GammaBlue, 255));
    const plColor rhs(plColorGammaUB(cpr.m_GammaRed, cpr.m_GammaGreen, cpr.m_GammaBlue, 255));

    /// \todo Use a midpoint interpolation

    // interpolate (linear for now)
    const float lerpX = (float)(x - cpl.m_PosX) * cpl.m_fInvDistToNextCp;

    ref_rgb = plMath::Lerp(lhs, rhs, lerpX);
  }
  }
  else if (m_ColorCPs.GetCount() == 1)
  {
    ref_rgb = plColorGammaUB(m_ColorCPs[0].m_GammaRed, m_ColorCPs[0].m_GammaGreen, m_ColorCPs[0].m_GammaBlue);
  }
}

void plColorGradient::EvaluateAlpha(double x, plUInt8& ref_uiAlpha) const
{
  ref_uiAlpha = 255;

  const plUInt32 numCPs = m_AlphaCPs.GetCount();
  if (numCPs >= 2)
  {
    // clamp to left value
    if (m_AlphaCPs[0].m_PosX >= x)
    {
      ref_uiAlpha = m_AlphaCPs[0].m_Alpha;
      return;
    }

    plUInt32 uiControlPoint;

    for (plUInt32 i = 1; i < numCPs; ++i)
    {
      if (m_AlphaCPs[i].m_PosX >= x)
      {
        uiControlPoint = i - 1;
        goto found;
      }
    }

    // no point found -> clamp to right value
    {
      ref_uiAlpha = m_AlphaCPs[numCPs - 1].m_Alpha;
      return;
    }

  found:
  {
    /// \todo Use a midpoint interpolation

    const AlphaCP& cpl = m_AlphaCPs[uiControlPoint];
    const AlphaCP& cpr = m_AlphaCPs[uiControlPoint + 1];

    // interpolate (linear for now)
    const float lerpX = (float)(x - cpl.m_PosX) * cpl.m_fInvDistToNextCp;

    ref_uiAlpha = plMath::Lerp(cpl.m_Alpha, cpr.m_Alpha, lerpX);
  }
  }
  else if (m_AlphaCPs.GetCount() == 1)
  {
    ref_uiAlpha = m_AlphaCPs[0].m_Alpha;
  }
}

void plColorGradient::EvaluateIntensity(double x, float& ref_fIntensity) const
{
  ref_fIntensity = 1.0f;

  const plUInt32 numCPs = m_IntensityCPs.GetCount();
  if (m_IntensityCPs.GetCount() >= 2)
  {
    // clamp to left value
    if (m_IntensityCPs[0].m_PosX >= x)
    {
      ref_fIntensity = m_IntensityCPs[0].m_Intensity;
      return;
    }

    plUInt32 uiControlPoint;

    for (plUInt32 i = 1; i < numCPs; ++i)
    {
      if (m_IntensityCPs[i].m_PosX >= x)
      {
        uiControlPoint = i - 1;
        goto found;
      }
    }

    // no point found -> clamp to right value
    {
      ref_fIntensity = m_IntensityCPs[numCPs - 1].m_Intensity;
      return;
    }

  found:
  {
    const IntensityCP& cpl = m_IntensityCPs[uiControlPoint];
    const IntensityCP& cpr = m_IntensityCPs[uiControlPoint + 1];

    /// \todo Use a midpoint interpolation

    // interpolate (linear for now)
    const float lerpX = (float)(x - cpl.m_PosX) * cpl.m_fInvDistToNextCp;

    ref_fIntensity = plMath::Lerp(cpl.m_Intensity, cpr.m_Intensity, lerpX);
  }
  }
  else if (m_IntensityCPs.GetCount() == 1)
  {
    ref_fIntensity = m_IntensityCPs[0].m_Intensity;
  }
}

plUInt64 plColorGradient::GetHeapMemoryUsage() const
{
  return m_ColorCPs.GetHeapMemoryUsage() + m_AlphaCPs.GetHeapMemoryUsage() + m_IntensityCPs.GetHeapMemoryUsage();
}

void plColorGradient::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 2;

  inout_stream << uiVersion;

  const plUInt32 numColor = m_ColorCPs.GetCount();
  const plUInt32 numAlpha = m_AlphaCPs.GetCount();
  const plUInt32 numIntensity = m_IntensityCPs.GetCount();

  inout_stream << numColor;
  inout_stream << numAlpha;
  inout_stream << numIntensity;

  for (const auto& cp : m_ColorCPs)
  {
    inout_stream << cp.m_PosX;
    inout_stream << cp.m_GammaRed;
    inout_stream << cp.m_GammaGreen;
    inout_stream << cp.m_GammaBlue;
  }

  for (const auto& cp : m_AlphaCPs)
  {
    inout_stream << cp.m_PosX;
    inout_stream << cp.m_Alpha;
  }

  for (const auto& cp : m_IntensityCPs)
  {
    inout_stream << cp.m_PosX;
    inout_stream << cp.m_Intensity;
  }
}

void plColorGradient::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;

  inout_stream >> uiVersion;
  PL_ASSERT_DEV(uiVersion <= 2, "Incorrect version '{0}' for plColorGradient", uiVersion);

  plUInt32 numColor = 0;
  plUInt32 numAlpha = 0;
  plUInt32 numIntensity = 0;

  inout_stream >> numColor;
  inout_stream >> numAlpha;
  inout_stream >> numIntensity;

  m_ColorCPs.SetCountUninitialized(numColor);
  m_AlphaCPs.SetCountUninitialized(numAlpha);
  m_IntensityCPs.SetCountUninitialized(numIntensity);

  if (uiVersion == 1)
  {
    float x;
    for (auto& cp : m_ColorCPs)
    {
      inout_stream >> x;
      cp.m_PosX = x; // float
      inout_stream >> cp.m_GammaRed;
      inout_stream >> cp.m_GammaGreen;
      inout_stream >> cp.m_GammaBlue;
    }

    for (auto& cp : m_AlphaCPs)
    {
      inout_stream >> x;
      cp.m_PosX = x; // float
      inout_stream >> cp.m_Alpha;
    }

    for (auto& cp : m_IntensityCPs)
    {
      inout_stream >> x;
      cp.m_PosX = x; // float
      inout_stream >> cp.m_Intensity;
    }
  }
  else
  {
    for (auto& cp : m_ColorCPs)
    {
      inout_stream >> cp.m_PosX;
      inout_stream >> cp.m_GammaRed;
      inout_stream >> cp.m_GammaGreen;
      inout_stream >> cp.m_GammaBlue;
    }

    for (auto& cp : m_AlphaCPs)
    {
      inout_stream >> cp.m_PosX;
      inout_stream >> cp.m_Alpha;
    }

    for (auto& cp : m_IntensityCPs)
    {
      inout_stream >> cp.m_PosX;
      inout_stream >> cp.m_Intensity;
    }
  }

  PrecomputeLerpNormalizer();
}


