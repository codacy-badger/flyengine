#pragma once

#include <Foundation/SimdMath/SimdBSphere.h>

class plSimdBBox
{
public:
  PL_DECLARE_POD_TYPE();

  /// \brief Default constructor does not initialize anything.
  plSimdBBox();

  /// \brief Constructs the box with the given minimum and maximum values.
  plSimdBBox(const plSimdVec4f& vMin, const plSimdVec4f& vMax); // [tested]

  /// \brief Creates a box that is located at the origin and has zero size. This is a 'valid' box.
  [[nodiscard]] static plSimdBBox MakeZero();

  /// \brief Creates a box that is in an invalid state. ExpandToInclude can then be used to make it into a bounding box for objects.
  [[nodiscard]] static plSimdBBox MakeInvalid(); // [tested]

  /// \brief Creates a box from a center point and half-extents for each axis.
  [[nodiscard]] static plSimdBBox MakeFromCenterAndHalfExtents(const plSimdVec4f& vCenter, const plSimdVec4f& vHalfExtents); // [tested]

  /// \brief Creates a box with the given minimum and maximum values.
  [[nodiscard]] static plSimdBBox MakeFromMinMax(const plSimdVec4f& vMin, const plSimdVec4f& vMax); // [tested]

  /// \brief Creates a box around the given set of points. If uiNumPoints is zero, the returned box is invalid (same as MakeInvalid() returns).
  [[nodiscard]] static plSimdBBox MakeFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plSimdVec4f)); // [tested]

public:
  /// \brief Resets the box to an invalid state. ExpandToInclude can then be used to make it into a bounding box for objects.
  [[deprecated("Use MakeInvalid() instead.")]] void SetInvalid(); // [tested]

  /// \brief Sets the box from a center point and half-extents for each axis.
  [[deprecated("Use MakeFromCenterAndHalfExtents() instead.")]] void SetCenterAndHalfExtents(const plSimdVec4f& vCenter, const plSimdVec4f& vHalfExtents); // [tested]

  /// \brief Creates a new bounding-box around the given set of points.
  [[deprecated("Use MakeFromPoints() instead.")]] void SetFromPoints(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plSimdVec4f)); // [tested]

  /// \brief Checks whether the box is in an invalid state.
  bool IsValid() const; // [tested]

  /// \brief Checks whether any component is NaN.
  bool IsNaN() const; // [tested]

  /// \brief Returns the center position of the box.
  plSimdVec4f GetCenter() const; // [tested]

  /// \brief Returns the extents of the box along each axis.
  plSimdVec4f GetExtents() const; // [tested]

  /// \brief Returns the half extents of the box along each axis.
  plSimdVec4f GetHalfExtents() const; // [tested]

  /// \brief Expands the box such that the given point is inside it.
  void ExpandToInclude(const plSimdVec4f& vPoint); // [tested]

  /// \brief Expands the box such that all the given points are inside it.
  void ExpandToInclude(const plSimdVec4f* pPoints, plUInt32 uiNumPoints, plUInt32 uiStride = sizeof(plSimdVec4f)); // [tested]

  /// \brief Expands the box such that the given box is inside it.
  void ExpandToInclude(const plSimdBBox& rhs); // [tested]

  /// \brief If the box is not cubic all extents are set to the value of the maximum extent, such that the box becomes cubic.
  void ExpandToCube(); // [tested]


  /// \brief Checks whether the given point is inside the box.
  bool Contains(const plSimdVec4f& vPoint) const; // [tested]

  /// \brief Checks whether the given box is completely inside this box.
  bool Contains(const plSimdBBox& rhs) const; // [tested]

  /// \brief Checks whether the given sphere is completely inside this box.
  bool Contains(const plSimdBSphere& sphere) const; // [tested]

  /// \brief Checks whether this box overlaps with the given box.
  bool Overlaps(const plSimdBBox& rhs) const; // [tested]

  /// \brief Checks whether the given sphere overlaps with this box.
  bool Overlaps(const plSimdBSphere& sphere) const; // [tested]


  /// \brief Will increase the size of the box in all directions by the given amount (per axis).
  void Grow(const plSimdVec4f& vDiff); // [tested]

  /// \brief Moves the box by the given vector.
  void Translate(const plSimdVec4f& vDiff); // [tested]

  /// \brief Transforms the corners of the box and recomputes the aabb of those transformed points.
  void Transform(const plSimdTransform& transform); // [tested]

  /// \brief Transforms the corners of the box and recomputes the aabb of those transformed points.
  void Transform(const plSimdMat4f& mMat); // [tested]


  /// \brief The given point is clamped to the volume of the box, i.e. it will be either inside the box or on its surface and it will have the closest
  /// possible distance to the original point.
  plSimdVec4f GetClampedPoint(const plSimdVec4f& vPoint) const; // [tested]

  /// \brief Returns the squared minimum distance from the box's surface to the point. Zero if the point is inside the box.
  plSimdFloat GetDistanceSquaredTo(const plSimdVec4f& vPoint) const; // [tested]

  /// \brief Returns the minimum distance from the box's surface to the point. Zero if the point is inside the box.
  plSimdFloat GetDistanceTo(const plSimdVec4f& vPoint) const; // [tested]


  bool operator==(const plSimdBBox& rhs) const; // [tested]
  bool operator!=(const plSimdBBox& rhs) const; // [tested]

public:
  plSimdVec4f m_Min;
  plSimdVec4f m_Max;
};

#include <Foundation/SimdMath/Implementation/SimdBBox_inl.h>
