#pragma once

#include <JoltPlugin/Constraints/JoltConstraintComponent.h>

using plJoltSwingTwistConstraintComponentManager = plComponentManager<class plJoltSwingTwistConstraintComponent, plBlockStorageType::Compact>;

/// \brief Implements a swing-twist physics constraint.
///
/// This is similar to a cone constraint but with more control.
/// The swing angle can be limited along Y and Z, so it can have a squashed shape, rather than perfectly round.
/// Additionally it can be limited how far the child actor may twist around the main axis.
class PL_JOLTPLUGIN_DLL plJoltSwingTwistConstraintComponent : public plJoltConstraintComponent
{
  PL_DECLARE_COMPONENT_TYPE(plJoltSwingTwistConstraintComponent, plJoltConstraintComponent, plJoltSwingTwistConstraintComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltConstraintComponent

protected:
  virtual void CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1) override;
  virtual void ApplySettings() final override;
  virtual bool ExceededBreakingPoint() final override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltSwingTwistConstraintComponent

public:
  plJoltSwingTwistConstraintComponent();
  ~plJoltSwingTwistConstraintComponent();

  /// \brief Sets how far the child actor may swing along the Y axis.
  void SetSwingLimitY(plAngle f);                          // [ property ]
  plAngle GetSwingLimitY() const { return m_SwingLimitY; } // [ property ]

  /// \brief Sets how far the child actor may swing along the Z axis.
  void SetSwingLimitZ(plAngle f);                          // [ property ]
  plAngle GetSwingLimitZ() const { return m_SwingLimitZ; } // [ property ]

  /// \brief Sets how difficult it is to rotate the constraint.
  void SetFriction(float f);                        // [ property ]
  float GetFriction() const { return m_fFriction; } // [ property ]

  /// \brief Sets how far the child actor can rotate around the twist axis in one direction.
  void SetLowerTwistLimit(plAngle f);                              // [ property ]
  plAngle GetLowerTwistLimit() const { return m_LowerTwistLimit; } // [ property ]

  /// \brief Sets how far the child actor can rotate around the twist axis in the other direction.
  void SetUpperTwistLimit(plAngle f);                              // [ property ]
  plAngle GetUpperTwistLimit() const { return m_UpperTwistLimit; } // [ property ]

  // void SetTwistDriveMode(plJoltConstraintDriveMode::Enum mode);                          // [ property ]
  // plJoltConstraintDriveMode::Enum GetTwistDriveMode() const { return m_TwistDriveMode; } // [ property ]

  // void SetTwistDriveTargetValue(plAngle f);                                    // [ property ]
  // plAngle GetTwistDriveTargetValue() const { return m_TwistDriveTargetValue; } // [ property ]

  // void SetTwistDriveStrength(float f);                                  // [ property ]
  // float GetTwistDriveStrength() const { return m_fTwistDriveStrength; } // [ property ]

protected:
  plAngle m_SwingLimitY;
  plAngle m_SwingLimitZ;

  float m_fFriction = 0.0f;

  plAngle m_LowerTwistLimit = plAngle::MakeFromDegree(90);
  plAngle m_UpperTwistLimit = plAngle::MakeFromDegree(90);

  // not sure whether these are useful
  // maybe just expose an 'untwist' feature, with strength/frequency and drive to position 0 ?
  // driving to velocity makes no sense, since the constraint always has a lower/upper twist limit
  // probably would need a 6DOF joint for more advanced use cases
  // plEnum<plJoltConstraintDriveMode> m_TwistDriveMode;
  // plAngle m_TwistDriveTargetValue;
  // float m_fTwistDriveStrength = 0; // 0 means maximum strength
};
