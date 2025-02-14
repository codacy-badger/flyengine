#pragma once

#include <Foundation/Math/Math.h>

#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
#  define PL_VEC2_CHECK_FOR_NAN(obj) (obj)->AssertNotNaN();
#else
#  define PL_VEC2_CHECK_FOR_NAN(obj)
#endif

/// \brief A 2-component vector class.
template <typename Type>
class plVec2Template
{
public:
  // Means that vectors can be copied using memcpy instead of copy construction.
  PL_DECLARE_POD_TYPE();

  using ComponentType = Type;


  // *** Data ***
public:
  Type x;
  Type y;

  // *** Constructors ***
public:
  /// \brief default-constructed vector is uninitialized (for speed)
  plVec2Template(); // [tested]

  /// \brief Initializes the vector with x,y
  plVec2Template(Type x, Type y); // [tested]

  /// \brief Initializes all components with xy
  explicit plVec2Template(Type v); // [tested]

  // no copy-constructor and operator= since the default-generated ones will be faster

  /// \brief Returns a vector with all components set to Not-a-Number (NaN).
  PL_DECLARE_IF_FLOAT_TYPE
  [[nodiscard]] static const plVec2Template<Type> MakeNaN()
  {
    return plVec2Template<Type>(plMath::NaN<Type>());
  }

  /// \brief Static function that returns a zero-vector.
  [[nodiscard]] static constexpr plVec2Template<Type> MakeZero() { return plVec2Template(0); } // [tested]

#if PL_ENABLED(PL_MATH_CHECK_FOR_NAN)
  void AssertNotNaN() const
  {
    PL_ASSERT_ALWAYS(!IsNaN(), "This object contains NaN values. This can happen when you forgot to initialize it before using it. Please "
                               "check that all code-paths properly initialize this object.");
  }
#endif

  // *** Conversions ***
public:
  /// \brief Returns an plVec3Template with x,y from this vector and z set by the parameter.
  const plVec3Template<Type> GetAsVec3(Type z) const; // [tested]

  /// \brief Returns an plVec4Template with x,y from this vector and z and w set by the parameters.
  const plVec4Template<Type> GetAsVec4(Type z, Type w) const; // [tested]

  /// \brief Returns the data as an array.
  const Type* GetData() const { return &x; }

  /// \brief Returns the data as an array.
  Type* GetData() { return &x; }

  // *** Functions to set the vector to specific values ***
public:
  /// \brief Sets all components to this value.
  void Set(Type xy); // [tested]

  /// \brief Sets the vector to these values.
  void Set(Type x, Type y); // [tested]

  /// \brief Sets the vector to all zero.
  void SetZero(); // [tested]

  // *** Functions dealing with length ***
public:
  /// \brief Returns the length of the vector.
  PL_DECLARE_IF_FLOAT_TYPE
  Type GetLength() const; // [tested]

  /// \brief Tries to rescale the vector to the given length. If the vector is too close to zero, PL_FAILURE is returned and the vector is
  /// set to zero.
  PL_DECLARE_IF_FLOAT_TYPE
  plResult SetLength(Type fNewLength, Type fEpsilon = plMath::DefaultEpsilon<Type>()); // [tested]

  /// \brief Returns the squared length. Faster, since no square-root is taken. Useful, if one only wants to compare the lengths of two
  /// vectors.
  Type GetLengthSquared() const; // [tested]

  /// \brief Normalizes this vector and returns its previous length in one operation. More efficient than calling GetLength and then
  /// Normalize.
  PL_DECLARE_IF_FLOAT_TYPE
  Type GetLengthAndNormalize(); // [tested]

  /// \brief Returns a normalized version of this vector, leaves the vector itself unchanged.
  PL_DECLARE_IF_FLOAT_TYPE
  const plVec2Template<Type> GetNormalized() const; // [tested]

  /// \brief Normalizes this vector.
  PL_DECLARE_IF_FLOAT_TYPE
  void Normalize(); // [tested]

  /// \brief Tries to normalize this vector. If the vector is too close to zero, PL_FAILURE is returned and the vector is set to the given
  /// fallback value.
  PL_DECLARE_IF_FLOAT_TYPE
  plResult NormalizeIfNotZero(const plVec2Template<Type>& vFallback = plVec2Template<Type>(1, 0), Type fEpsilon = plMath::DefaultEpsilon<Type>()); // [tested]

  /// \brief Returns, whether this vector is (0, 0).
  bool IsZero() const; // [tested]

  /// \brief Returns, whether this vector is (0, 0) within a certain threshold.
  bool IsZero(Type fEpsilon) const; // [tested]

  /// \brief Returns, whether the squared length of this vector is between 0.999f and 1.001f.
  PL_DECLARE_IF_FLOAT_TYPE
  bool IsNormalized(Type fEpsilon = plMath::HugeEpsilon<Type>()) const; // [tested]

  /// \brief Returns true, if any of x or y is NaN
  bool IsNaN() const; // [tested]

  /// \brief Checks that all components are finite numbers.
  bool IsValid() const; // [tested]


  // *** Operators ***
public:
  /// \brief Returns the negation of this vector.
  const plVec2Template<Type> operator-() const; // [tested]

  /// \brief Adds cc component-wise to this vector
  void operator+=(const plVec2Template<Type>& vCc); // [tested]

  /// \brief Subtracts cc component-wise from this vector
  void operator-=(const plVec2Template<Type>& vCc); // [tested]

  /// \brief Multiplies all components of this vector with f
  void operator*=(Type f); // [tested]

  /// \brief Divides all components of this vector by f
  void operator/=(Type f); // [tested]

  /// \brief Equality Check (bitwise)
  bool IsIdentical(const plVec2Template<Type>& rhs) const; // [tested]

  /// \brief Equality Check with epsilon
  bool IsEqual(const plVec2Template<Type>& rhs, Type fEpsilon) const; // [tested]


  // *** Common vector operations ***
public:
  /// \brief Returns the positive angle between *this and rhs.
  plAngle GetAngleBetween(const plVec2Template<Type>& rhs) const; // [tested]

  /// \brief Returns the Dot-product of the two vectors (commutative, order does not matter)
  Type Dot(const plVec2Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise minimum of *this and rhs
  const plVec2Template<Type> CompMin(const plVec2Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise maximum of *this and rhs
  const plVec2Template<Type> CompMax(const plVec2Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise clamped value of *this between low and high.
  const plVec2Template<Type> CompClamp(const plVec2Template<Type>& vLow, const plVec2Template<Type>& vHigh) const; // [tested]

  /// \brief Returns the component-wise multiplication of *this and rhs
  const plVec2Template<Type> CompMul(const plVec2Template<Type>& rhs) const; // [tested]

  /// \brief Returns the component-wise division of *this and rhs
  const plVec2Template<Type> CompDiv(const plVec2Template<Type>& rhs) const; // [tested]

  /// brief Returns the component-wise absolute of *this.
  const plVec2Template<Type> Abs() const; // [tested]


  // *** Other common operations ***
public:
  /// \brief Modifies this direction vector to be orthogonal to the given (normalized) direction vector. The result is NOT normalized.
  ///
  /// \note This function may fail, e.g. create a vector that is zero, if the given normal is parallel to the vector itself.
  ///       If you need to handle such cases, you should manually check afterwards, whether the result is zero, or cannot be normalized.
  PL_DECLARE_IF_FLOAT_TYPE
  void MakeOrthogonalTo(const plVec2Template<Type>& vNormal); // [tested]

  /// \brief Returns some arbitrary vector orthogonal to this one. The vector is NOT normalized.
  const plVec2Template<Type> GetOrthogonalVector() const; // [tested]

  /// \brief Returns this vector reflected at vNormal.
  PL_DECLARE_IF_FLOAT_TYPE
  const plVec2Template<Type> GetReflectedVector(const plVec2Template<Type>& vNormal) const; // [tested]
};

// *** Operators ***

/// \brief Component-wise addition.
template <typename Type>
const plVec2Template<Type> operator+(const plVec2Template<Type>& v1, const plVec2Template<Type>& v2); // [tested]

/// \brief Component-wise subtraction.
template <typename Type>
const plVec2Template<Type> operator-(const plVec2Template<Type>& v1, const plVec2Template<Type>& v2); // [tested]

/// \brief Returns a scaled vector.
template <typename Type>
const plVec2Template<Type> operator*(Type f, const plVec2Template<Type>& v); // [tested]

/// \brief Returns a scaled vector.
template <typename Type>
const plVec2Template<Type> operator*(const plVec2Template<Type>& v, Type f); // [tested]

/// \brief Returns a scaled vector.
template <typename Type>
const plVec2Template<Type> operator/(const plVec2Template<Type>& v, Type f); // [tested]

/// \brief Returns true, if both vectors are identical.
template <typename Type>
bool operator==(const plVec2Template<Type>& v1, const plVec2Template<Type>& v2); // [tested]

/// \brief Returns true, if both vectors are not identical.
template <typename Type>
bool operator!=(const plVec2Template<Type>& v1, const plVec2Template<Type>& v2); // [tested]

/// \brief Strict weak ordering. Useful for sorting vertices into a map.
template <typename Type>
bool operator<(const plVec2Template<Type>& v1, const plVec2Template<Type>& v2);

#include <Foundation/Math/Implementation/Vec2_inl.h>
