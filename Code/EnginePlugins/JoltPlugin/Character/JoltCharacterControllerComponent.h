#pragma once

#include <Core/World/World.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>

struct plMsgMoveCharacterController;
struct plMsgUpdateLocalBounds;

namespace JPH
{
  class CharacterVirtual;
  class TempAllocator;
} // namespace JPH

PL_DECLARE_FLAGS(plUInt32, plJoltCharacterDebugFlags, PrintState, VisShape, VisContacts, VisCasts, VisGroundContact, VisFootCheck);
PL_DECLARE_REFLECTABLE_TYPE(PL_JOLTPLUGIN_DLL, plJoltCharacterDebugFlags);

/// \brief Base class for character controllers (CC).
///
/// This class provides general functionality for building a character controller.
/// It tries not to implement things that are game specific.
/// It is assumed that most games implement their own character controller to be able to build very specific behavior.
/// The plJoltDefaultCharacterComponent is an example implementation that shows how this can be achieved on top of this class.
class PL_JOLTPLUGIN_DLL plJoltCharacterControllerComponent : public plCharacterControllerComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plJoltCharacterControllerComponent, plCharacterControllerComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plJoltCharacterControllerComponent

public:
  plJoltCharacterControllerComponent();
  ~plJoltCharacterControllerComponent();

  /// \brief Describes a point where the CC collided with other geometry.
  struct ContactPoint
  {
    float m_fCastFraction = 0.0f;
    plVec3 m_vPosition;
    plVec3 m_vSurfaceNormal;
    plVec3 m_vContactNormal;
    JPH::BodyID m_BodyID;
    JPH::SubShapeID m_SubShapeID;
  };

  /// \brief The CC will move through the given physics body.
  ///
  /// Currently only one such object can be set. This is mainly used to ignore an object that the player is currently carrying,
  /// so that there are no unintended collisions.
  ///
  /// Call ClearObjectToIgnore() to re-enable collisions.
  void SetObjectToIgnore(plUInt32 uiObjectFilterID);

  /// \see SetObjectToIgnore()
  void ClearObjectToIgnore();

public:
  /// The collision layer determines with which other actors this actor collides. \see plJoltActorComponent
  plUInt8 m_uiCollisionLayer = 0; // [ property ]

  /// In case a 'presence shape' is used, this defines which geometry the presence bodies collides with.
  plUInt8 m_uiPresenceCollisionLayer = 0; // [ property ]

  /// What aspects of the CC to visualize.
  plBitflags<plJoltCharacterDebugFlags> m_DebugFlags; // [ property ]

  /// \brief The maximum slope that the character can walk up.
  void SetMaxClimbingSlope(plAngle slope);                           // [ property ]
  plAngle GetMaxClimbingSlope() const { return m_MaxClimbingSlope; } // [ property ]

  /// \brief The mass with which the character will push down on objects that it is standing on.
  void SetMass(float fMass);                // [ property ]
  float GetMass() const { return m_fMass; } // [ property ]

  /// \brief The strength with which the character will push against objects that it is running into.
  void SetStrength(float fStrength);                // [ property ]
  float GetStrength() const { return m_fStrength; } // [ property ]

   /// \brief Attempts to move the character into the given direction.
  ///
  /// Implements the specific constraints, such as colliding with geometry and sliding along walls.
  /// Should NOT add further functionality, such as gravity. This function applies the result immediately
  /// and moves the owner object to the final location.
  virtual void RawMove(const plVec3& vMoveDeltaGlobal) override{}; // [ scriptable ]

  /// \brief Instructs the CC to move in certain directions. An implementation can queue the request for later processing.
  ///
  /// It can also add further functionality, such as adding gravity, stair stepping, etc.
  virtual void MoveCharacter(plMsgMoveCharacterController& msg) override{}; // [ msg handler ]

  /// \brief Teleports the CC to the desired global position. Ignores obstacles on the path.
  ///
  /// Careful, the CC may get stuck in the new location, if it is inside static geometry.
  /// If it teleports into dynamic geometry, the result may also be undesirable.
  virtual void TeleportCharacter(const plVec3& vGlobalFootPos) override{}; // [ scriptable ]

  /// \brief Checks whether the CC can be teleported to the target position without getting stuck.
  ///
  /// This can be used to check before using TeleportCharacter(). It can also be used to check whether a character
  /// can stand up from a crouching position, by passing a non-zero character height.
  ///
  /// If a character height of 0 is passed in, the current height is used.
  virtual bool IsDestinationUnobstructed(const plVec3& vGlobalFootPos, float fCharacterHeight) override { return false; }; // [ scriptable ]

  /// \brief Checks whether the CC is currently touching the ground.
  virtual bool IsTouchingGround() override { return true; }; // [ scriptable ]

  /// \brief Checks whether the CC is currently in the crouch state.
  virtual bool IsCrouching() override { return false; }; // [ scriptable ]

private:
  plAngle m_MaxClimbingSlope = plAngle::MakeFromDegree(45); // [ property ]
  float m_fMass = 70.0f;                                    // [ property ]
  float m_fStrength = 500.0f;                               // [ property ]

protected:
  /// \brief Returns the time delta to use for updating the character. This may differ from the world delta.
  PL_ALWAYS_INLINE float GetUpdateTimeDelta() const { return m_fUpdateTimeDelta; }

  /// \brief Returns the inverse of update time delta.
  PL_ALWAYS_INLINE float GetInverseUpdateTimeDelta() const { return m_fInverseUpdateTimeDelta; }

  /// \brief Returns the shape that the character is supposed to use next.
  ///
  /// The desired target state (radius, height, etc) has to be stored somewhere else (e.g. as members in derived classes).
  /// The shape can be cached.
  /// The shape may not get applied to the character, in case this is used by things like TryResize and the next shape is
  /// determined to not fit.
  virtual JPH::Ref<JPH::Shape> MakeNextCharacterShape() = 0;

  /// \brief Returns the radius of the shape. This never changes at runtime.
  virtual float GetShapeRadius() const = 0;

  /// \brief Called up to once per frame, but potentially less often, if physics updates were skipped due to high framerates.
  ///
  /// All shape modifications and moves should only be executed during this step.
  /// The given deltaTime should be used, rather than the world's time diff.
  virtual void UpdateCharacter() = 0;

  /// \brief Gives access to the internally used JPH::CharacterVirtual.
  JPH::CharacterVirtual* GetJoltCharacter() { return m_pCharacter; }
  const JPH::CharacterVirtual* GetJoltCharacter() const { return m_pCharacter; }

  /// \brief Attempts to change the character shape to the new one. Fails if the new shape overlaps with surrounding geometry.
  plResult TryChangeShape(JPH::Shape* pNewShape);

  /// \brief Moves the character using the given velocity and timestep, making it collide with and slide along obstacles.
  void RawMoveWithVelocity(const plVec3& vVelocity, float fMaxStairStepUp, float fMaxStepDown);

  /// \brief Variant of RawMoveWithVelocity() that takes a direction vector instead.
  void RawMoveIntoDirection(const plVec3& vDirection);

  /// \brief Variant of RawMoveWithVelocity() that takes a target position instead.
  void RawMoveToPosition(const plVec3& vTargetPosition);

  /// \brief Teleports the character to the destination position, even if it would get stuck there.
  void TeleportToPosition(const plVec3& vGlobalFootPos);

  /// \brief If the CC is slightly above the ground, this will move it down so that it touches the ground.
  ///
  /// If within the max distance no ground contact is found, the function does nothing and returns false.
  bool StickToGround(float fMaxDist);

  /// \brief Gathers all contact points that are found by sweeping the shape along a direction
  void CollectCastContacts(plDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const plVec3& vQueryPosition, const plQuat& qQueryRotation, const plVec3& vSweepDir) const;

  /// \brief Gathers all contact points of the shape at the target position.
  ///
  /// Use fCollisionTolerance > 0 (e.g. 0.02f) to find contacts with walls/ground that the shape is touching but not penetrating.
  void CollectContacts(plDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const plVec3& vQueryPosition, const plQuat& qQueryRotation, float fCollisionTolerance) const;

  /// \brief Detects the velocity at the contact point. If it is a dynamic body, a force pushing it away is applied.
  ///
  /// This is mainly used to get the velocity of the kinematic object that a character is standing on.
  /// It can then be incorporated into the movement, such that the character rides along.
  /// If the body at the contact point is dynamic, optionally a force can be applied, simulating that the character's
  /// weight pushes down on it.
  plVec3 GetContactVelocityAndPushAway(const ContactPoint& contact, float fPushForce);

  /// \brief Spawns a surface interaction prefab at the given contact point.
  ///
  /// hFallbackSurface is used, if no other surface could be determined from the contact point.
  void SpawnContactInteraction(const ContactPoint& contact, const plHashedString& sSurfaceInteraction, plSurfaceResourceHandle hFallbackSurface, const plVec3& vInteractionNormal = plVec3(0, 0, 1));

  /// \brief Debug draws the contact point.
  void VisualizeContact(const ContactPoint& contact, const plColor& color) const;

  /// \brief Debug draws all the contact points.
  void VisualizeContacts(const plDynamicArray<ContactPoint>& contacts, const plColor& color) const;

private:
  friend class plJoltWorldModule;

  void Update(plTime deltaTime);

  float m_fUpdateTimeDelta = 0.1f;
  float m_fInverseUpdateTimeDelta = 1.0f;
  JPH::CharacterVirtual* m_pCharacter = nullptr;

  void CreatePresenceBody();
  void RemovePresenceBody();
  void MovePresenceBody(plTime deltaTime);

  plUInt32 m_uiPresenceBodyID = plInvalidIndex;
  plUInt32 m_uiPresenceBodyAddCounter = 0;

  plJoltBodyFilter m_BodyFilter;
  plUInt32 m_uiUserDataIndex = plInvalidIndex;
};
