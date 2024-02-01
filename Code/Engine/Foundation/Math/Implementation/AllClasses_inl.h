#pragma once

#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/BoundingSphere.h>
#include <Foundation/Math/Mat3.h>
#include <Foundation/Math/Mat4.h>
#include <Foundation/Math/Plane.h>

template <typename Type>
PL_ALWAYS_INLINE bool plBoundingBoxTemplate<Type>::Contains(const plBoundingSphereTemplate<Type>& sphere) const
{
  return Contains(sphere.GetBoundingBox());
}

template <typename Type>
PL_ALWAYS_INLINE bool plBoundingBoxTemplate<Type>::Overlaps(const plBoundingSphereTemplate<Type>& sphere) const
{
  // check whether the closest point between box and sphere is inside the sphere (it is definitely inside the box)
  return sphere.Contains(GetClampedPoint(sphere.m_vCenter));
}

template <typename Type>
inline Type plBoundingBoxTemplate<Type>::GetDistanceTo(const plBoundingSphereTemplate<Type>& sphere) const
{
  return (GetClampedPoint(sphere.m_vCenter) - sphere.m_vCenter).GetLength() - sphere.m_fRadius;
}

template <typename Type>
inline const plBoundingSphereTemplate<Type> plBoundingBoxTemplate<Type>::GetBoundingSphere() const
{
  return plBoundingSphereTemplate<Type>::MakeFromCenterAndRadius(GetCenter(), (m_vMax - m_vMin).GetLength() * (Type)0.5);
}

template <typename Type>
void plBoundingSphereTemplate<Type>::ExpandToInclude(const plBoundingBoxTemplate<Type>& rhs)
{
  // compute the min and max extends of the AABB relative to the sphere (sphere center is the new origin)
  const plVec3 vDiffMax = rhs.m_vMax - m_vCenter;
  const plVec3 vDiffMin = rhs.m_vMin - m_vCenter;

  // compute the absolute distance to each AABB extremum, per axis
  const plVec3 vDiffMaxAbs(plMath::Abs(vDiffMax.x), plMath::Abs(vDiffMax.y), plMath::Abs(vDiffMax.z));
  const plVec3 vDiffMinAbs(plMath::Abs(vDiffMin.x), plMath::Abs(vDiffMin.y), plMath::Abs(vDiffMin.z));

  // take the maximum distance for each axis, to compute the point that is the farthest away from the sphere
  const plVec3 vMostDistantPoint = vDiffMinAbs.CompMax(vDiffMaxAbs);

  const Type fDistSQR = vMostDistantPoint.GetLengthSquared();

  if (plMath::Square(m_fRadius) < fDistSQR)
    m_fRadius = plMath::Sqrt(fDistSQR);
}

template <typename Type>
Type plBoundingSphereTemplate<Type>::GetDistanceTo(const plBoundingBoxTemplate<Type>& rhs) const
{
  const plVec3Template<Type> vPointOnBox = rhs.GetClampedPoint(m_vCenter);

  return GetDistanceTo(vPointOnBox);
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::Contains(const plBoundingBoxTemplate<Type>& rhs) const
{
  // compute the min and max extends of the AABB relative to the sphere (sphere center is the new origin)
  const plVec3 vDiffMax = rhs.m_vMax - m_vCenter;
  const plVec3 vDiffMin = rhs.m_vMin - m_vCenter;

  // compute the absolute distance to each AABB extremum, per axis
  const plVec3 vDiffMaxAbs(plMath::Abs(vDiffMax.x), plMath::Abs(vDiffMax.y), plMath::Abs(vDiffMax.z));
  const plVec3 vDiffMinAbs(plMath::Abs(vDiffMin.x), plMath::Abs(vDiffMin.y), plMath::Abs(vDiffMin.z));

  // take the maximum distance for each axis, to compute the point that is the farthest away from the sphere
  const plVec3 vMostDistantPoint = vDiffMinAbs.CompMax(vDiffMaxAbs);

  // if the squared length of that point is still smaller than the sphere radius, it is inside the sphere
  // and thus the whole AABB is inside the sphere
  return vMostDistantPoint.GetLengthSquared() <= m_fRadius * m_fRadius;
}

template <typename Type>
bool plBoundingSphereTemplate<Type>::Overlaps(const plBoundingBoxTemplate<Type>& rhs) const
{
  return Contains(rhs.GetClampedPoint(m_vCenter));
}

template <typename Type>
const plBoundingBoxTemplate<Type> plBoundingSphereTemplate<Type>::GetBoundingBox() const
{
  return plBoundingBoxTemplate<Type>::MakeFromMinMax(m_vCenter - plVec3Template<Type>(m_fRadius), m_vCenter + plVec3Template<Type>(m_fRadius));
}


template <typename Type>
plPositionOnPlane::Enum plPlaneTemplate<Type>::GetObjectPosition(const plBoundingSphereTemplate<Type>& sphere) const
{
  const Type fDist = GetDistanceTo(sphere.m_vCenter);

  if (fDist >= sphere.m_fRadius)
    return plPositionOnPlane::Front;

  if (-fDist >= sphere.m_fRadius)
    return plPositionOnPlane::Back;

  return plPositionOnPlane::Spanning;
}

template <typename Type>
plPositionOnPlane::Enum plPlaneTemplate<Type>::GetObjectPosition(const plBoundingBoxTemplate<Type>& box) const
{
  plVec3Template<Type> vPos = box.m_vMin;
  plVec3Template<Type> vNeg = box.m_vMax;

  if (m_vNormal.x >= (Type)0)
  {
    vPos.x = box.m_vMax.x;
    vNeg.x = box.m_vMin.x;
  }

  if (m_vNormal.y >= (Type)0)
  {
    vPos.y = box.m_vMax.y;
    vNeg.y = box.m_vMin.y;
  }

  if (m_vNormal.z >= (Type)0)
  {
    vPos.z = box.m_vMax.z;
    vNeg.z = box.m_vMin.z;
  }

  if (GetDistanceTo(vPos) <= (Type)0)
    return plPositionOnPlane::Back;

  if (GetDistanceTo(vNeg) >= (Type)0)
    return plPositionOnPlane::Front;

  return plPositionOnPlane::Spanning;
}

template <typename Type>
Type plPlaneTemplate<Type>::GetMinimumDistanceTo(const plBoundingBoxTemplate<Type>& box) const
{
  plVec3Template<Type> vNeg = box.m_vMax;

  if (m_vNormal.x >= (Type)0)
  {
    vNeg.x = box.m_vMin.x;
  }

  if (m_vNormal.y >= (Type)0)
  {
    vNeg.y = box.m_vMin.y;
  }

  if (m_vNormal.z >= (Type)0)
  {
    vNeg.z = box.m_vMin.z;
  }

  return GetDistanceTo(vNeg);
}

template <typename Type>
Type plPlaneTemplate<Type>::GetMaximumDistanceTo(const plBoundingBoxTemplate<Type>& box) const
{
  plVec3Template<Type> vPos = box.m_vMin;

  if (m_vNormal.x >= (Type)0)
  {
    vPos.x = box.m_vMax.x;
  }

  if (m_vNormal.y >= (Type)0)
  {
    vPos.y = box.m_vMax.y;
  }

  if (m_vNormal.z >= (Type)0)
  {
    vPos.z = box.m_vMax.z;
  }

  return GetDistanceTo(vPos);
}


template <typename Type>
plMat3Template<Type> plMat3Template<Type>::MakeAxisRotation(const plVec3Template<Type>& vAxis, plAngle angle)
{
  PL_ASSERT_DEBUG(vAxis.IsNormalized(0.1f), "vAxis must be normalized.");

  const Type cos = plMath::Cos(angle);
  const Type sin = plMath::Sin(angle);
  const Type oneminuscos = (Type)1 - cos;

  const Type xy = vAxis.x * vAxis.y;
  const Type xz = vAxis.x * vAxis.z;
  const Type yz = vAxis.y * vAxis.z;

  const Type xsin = vAxis.x * sin;
  const Type ysin = vAxis.y * sin;
  const Type zsin = vAxis.z * sin;

  const Type onecos_xy = oneminuscos * xy;
  const Type onecos_xz = oneminuscos * xz;
  const Type onecos_yz = oneminuscos * yz;

  plMat3Template<Type> res;

  // Column 1
  res.Element(0, 0) = cos + (oneminuscos * (vAxis.x * vAxis.x));
  res.Element(0, 1) = onecos_xy + zsin;
  res.Element(0, 2) = onecos_xz - ysin;

  // Column 2  )
  res.Element(1, 0) = onecos_xy - zsin;
  res.Element(1, 1) = cos + (oneminuscos * (vAxis.y * vAxis.y));
  res.Element(1, 2) = onecos_yz + xsin;

  // Column 3  )
  res.Element(2, 0) = onecos_xz + ysin;
  res.Element(2, 1) = onecos_yz - xsin;
  res.Element(2, 2) = cos + (oneminuscos * (vAxis.z * vAxis.z));

  return res;
}

template <typename Type>
plResult plMat3Template<Type>::Invert(Type fEpsilon)
{
  const Type fDet = Element(0, 0) * (Element(2, 2) * Element(1, 1) - Element(1, 2) * Element(2, 1)) -
                    Element(0, 1) * (Element(2, 2) * Element(1, 0) - Element(1, 2) * Element(2, 0)) +
                    Element(0, 2) * (Element(2, 1) * Element(1, 0) - Element(1, 1) * Element(2, 0));

  if (plMath::IsZero(fDet, fEpsilon))
    return PL_FAILURE;

  const Type fOneDivDet = (Type)1 / fDet;

  plMat3Template<Type> Inverse;

  Inverse.Element(0, 0) = (Element(2, 2) * Element(1, 1) - Element(1, 2) * Element(2, 1));
  Inverse.Element(0, 1) = -(Element(2, 2) * Element(0, 1) - Element(0, 2) * Element(2, 1));
  Inverse.Element(0, 2) = (Element(1, 2) * Element(0, 1) - Element(0, 2) * Element(1, 1));

  Inverse.Element(1, 0) = -(Element(2, 2) * Element(1, 0) - Element(1, 2) * Element(2, 0));
  Inverse.Element(1, 1) = (Element(2, 2) * Element(0, 0) - Element(0, 2) * Element(2, 0));
  Inverse.Element(1, 2) = -(Element(1, 2) * Element(0, 0) - Element(0, 2) * Element(1, 0));

  Inverse.Element(2, 0) = (Element(2, 1) * Element(1, 0) - Element(1, 1) * Element(2, 0));
  Inverse.Element(2, 1) = -(Element(2, 1) * Element(0, 0) - Element(0, 1) * Element(2, 0));
  Inverse.Element(2, 2) = (Element(1, 1) * Element(0, 0) - Element(0, 1) * Element(1, 0));

  *this = Inverse * fOneDivDet;
  return PL_SUCCESS;
}

template <typename Type>
plMat4Template<Type> plMat4Template<Type>::MakeAxisRotation(const plVec3Template<Type>& vAxis, plAngle angle)
{
  PL_ASSERT_DEBUG(vAxis.IsNormalized(), "vAxis must be normalized.");

  const Type cos = plMath::Cos(angle);
  const Type sin = plMath::Sin(angle);
  const Type oneminuscos = (Type)1 - cos;

  const Type xy = vAxis.x * vAxis.y;
  const Type xz = vAxis.x * vAxis.z;
  const Type yz = vAxis.y * vAxis.z;

  const Type xsin = vAxis.x * sin;
  const Type ysin = vAxis.y * sin;
  const Type zsin = vAxis.z * sin;

  const Type onecos_xy = oneminuscos * xy;
  const Type onecos_xz = oneminuscos * xz;
  const Type onecos_yz = oneminuscos * yz;

  plMat4Template<Type> res;

  // Column 1
  res.Element(0, 0) = cos + (oneminuscos * (vAxis.x * vAxis.x));
  res.Element(0, 1) = onecos_xy + zsin;
  res.Element(0, 2) = onecos_xz - ysin;
  res.Element(0, 3) = 0;

  // Column 2
  res.Element(1, 0) = onecos_xy - zsin;
  res.Element(1, 1) = cos + (oneminuscos * (vAxis.y * vAxis.y));
  res.Element(1, 2) = onecos_yz + xsin;
  res.Element(1, 3) = 0;

  // Column 3
  res.Element(2, 0) = onecos_xz + ysin;
  res.Element(2, 1) = onecos_yz - xsin;
  res.Element(2, 2) = cos + (oneminuscos * (vAxis.z * vAxis.z));
  res.Element(2, 3) = 0;

  // Column 4
  res.Element(3, 0) = 0;
  res.Element(3, 1) = 0;
  res.Element(3, 2) = 0;
  res.Element(3, 3) = 1;

  return res;
}

template <typename Type>
plResult plMat4Template<Type>::Invert(Type fEpsilon)
{
  plMat4Template<Type> Inverse;

  const Type fDet = GetDeterminantOf4x4Matrix(*this);

  if (plMath::IsZero(fDet, fEpsilon))
    return PL_FAILURE;

  Type fOneDivDet = plMath::Invert(fDet);

  for (plInt32 i = 0; i < 4; ++i)
  {

    Inverse.Element(i, 0) = GetDeterminantOf3x3SubMatrix(*this, i, 0) * fOneDivDet;
    fOneDivDet = -fOneDivDet;
    Inverse.Element(i, 1) = GetDeterminantOf3x3SubMatrix(*this, i, 1) * fOneDivDet;
    fOneDivDet = -fOneDivDet;
    Inverse.Element(i, 2) = GetDeterminantOf3x3SubMatrix(*this, i, 2) * fOneDivDet;
    fOneDivDet = -fOneDivDet;
    Inverse.Element(i, 3) = GetDeterminantOf3x3SubMatrix(*this, i, 3) * fOneDivDet;
  }

  *this = Inverse;
  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// static
template <typename T>
bool plComparisonOperator::Compare(plComparisonOperator::Enum cmp, const T& a, const T& b)
{
  switch (cmp)
  {
    case plComparisonOperator::Equal:
      return a == b;
    case plComparisonOperator::NotEqual:
      return !(a == b);
    case plComparisonOperator::Less:
      return a < b;
    case plComparisonOperator::LessEqual:
      return !(b < a);
    case plComparisonOperator::Greater:
      return b < a;
    case plComparisonOperator::GreaterEqual:
      return !(a < b);

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return false;
}
