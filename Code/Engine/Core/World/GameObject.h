#pragma once

/// \file

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/Time/Time.h>
#include <Foundation/Types/TagSet.h>

#include <Core/World/ComponentManager.h>
#include <Core/World/GameObjectDesc.h>

// Avoid conflicts with windows.h
#ifdef SendMessage
#  undef SendMessage
#endif

enum class plVisibilityState : plUInt8;

/// \brief This class represents an object inside the world.
///
/// Game objects only consists of hierarchical data like transformation and a list of components.
/// You cannot derive from the game object class. To add functionality to an object you have to attach components to it.
/// To create an object instance call CreateObject on the world. Never store a direct pointer to an object but store an
/// plGameObjectHandle instead.
///
/// \see plWorld
/// \see plComponent
/// \see plGameObjectHandle
class PL_CORE_DLL plGameObject final
{
private:
  enum
  {
#if PL_ENABLED(PL_PLATFORM_32BIT)
    NUM_INPLACE_COMPONENTS = 12
#else
    NUM_INPLACE_COMPONENTS = 6
#endif
  };

  friend class plWorld;
  friend class plInternal::WorldData;
  friend class plMemoryUtils;

  plGameObject();
  plGameObject(const plGameObject& other);
  ~plGameObject();

  void operator=(const plGameObject& other);

public:
  /// \brief Iterates over all children of one object.
  class PL_CORE_DLL ConstChildIterator
  {
  public:
    const plGameObject& operator*() const;
    const plGameObject* operator->() const;

    operator const plGameObject*() const;

    /// \brief Advances the iterator to the next child object. The iterator will not be valid anymore, if the last child is reached.
    void Next();

    /// \brief Checks whether this iterator points to a valid object.
    bool IsValid() const;

    /// \brief Shorthand for 'Next'
    void operator++();

  private:
    friend class plGameObject;

    ConstChildIterator(plGameObject* pObject, const plWorld* pWorld);

    plGameObject* m_pObject = nullptr;
    const plWorld* m_pWorld = nullptr;
  };

  class PL_CORE_DLL ChildIterator : public ConstChildIterator
  {
  public:
    plGameObject& operator*();
    plGameObject* operator->();

    operator plGameObject*();

  private:
    friend class plGameObject;

    ChildIterator(plGameObject* pObject, const plWorld* pWorld);
  };

  /// \brief Returns a handle to this object.
  plGameObjectHandle GetHandle() const;

  /// \brief Makes this object and all its children dynamic. Dynamic objects might move during runtime.
  void MakeDynamic();

  /// \brief Makes this object static. Static objects don't move during runtime.
  void MakeStatic();

  /// \brief Returns whether this object is dynamic.
  bool IsDynamic() const;

  /// \brief Returns whether this object is static.
  bool IsStatic() const;

  /// \brief Sets the 'active flag' of the game object, which affects its final 'active state'.
  ///
  /// The active flag affects the 'active state' of the game object and all its children and attached components.
  /// When a game object does not have the active flag, it is switched to 'inactive'. The same happens for all its children and
  /// all components attached to those game objects.
  /// Thus removing the active flag from a game object recursively deactivates the entire sub-tree of objects and components.
  ///
  /// When the active flag is set on a game object, and all of its parent nodes have the flag set as well, then the active state
  /// will be set to true on it and all its children and attached components.
  ///
  /// \sa IsActive(), plComponent::SetActiveFlag()
  void SetActiveFlag(bool bEnabled);

  /// \brief Checks whether the 'active flag' is set on this game object. Note that this does not mean that the game object is also in an 'active
  /// state'.
  ///
  /// \sa IsActive(), SetActiveFlag()
  bool GetActiveFlag() const;

  /// \brief Checks whether this game object is in an active state.
  ///
  /// The active state is determined by the active state of the parent game object and the 'active flag' of this game object.
  /// Only if the parent game object is active (and thus all of its parent objects as well) and this game object has the active flag set,
  /// will this game object be active.
  ///
  /// \sa plGameObject::SetActiveFlag(), plComponent::IsActive()
  bool IsActive() const;

  /// \brief Adds plObjectFlags::CreatedByPrefab to the object. See the flag for details.
  void SetCreatedByPrefab() { m_Flags.Add(plObjectFlags::CreatedByPrefab); }

  /// \brief Checks whether the plObjectFlags::CreatedByPrefab flag is set on this object.
  bool WasCreatedByPrefab() const { return m_Flags.IsSet(plObjectFlags::CreatedByPrefab); }

  /// \brief Sets the name to identify this object. Does not have to be a unique name.
  void SetName(plStringView sName);
  void SetName(const plHashedString& sName);
  plStringView GetName() const;
  bool HasName(const plTempHashedString& sName) const;

  /// \brief Sets the global key to identify this object. Global keys must be unique within a world.
  void SetGlobalKey(plStringView sGlobalKey);
  void SetGlobalKey(const plHashedString& sGlobalKey);
  plStringView GetGlobalKey() const;

  /// \brief Enables or disabled notification message 'plMsgChildrenChanged' when children are added or removed. The message is sent to this object and all its parent objects.
  void EnableChildChangesNotifications();
  void DisableChildChangesNotifications();

  /// \brief Enables or disabled notification message 'plMsgParentChanged' when the parent changes. The message is sent to this object only.
  void EnableParentChangesNotifications();
  void DisableParentChangesNotifications();

  /// \brief Defines during re-parenting what transform is going to be preserved.
  enum class TransformPreservation
  {
    PreserveLocal,
    PreserveGlobal
  };

  /// \brief Sets the parent of this object to the given.
  void SetParent(const plGameObjectHandle& hParent, plGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Gets the parent of this object or nullptr if this is a top-level object.
  plGameObject* GetParent();

  /// \brief Gets the parent of this object or nullptr if this is a top-level object.
  const plGameObject* GetParent() const;

  /// \brief Adds the given object as a child object.
  void AddChild(const plGameObjectHandle& hChild, plGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Adds the given objects as child objects.
  void AddChildren(const plArrayPtr<const plGameObjectHandle>& children, plGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Detaches the given child object from this object and makes it a top-level object.
  void DetachChild(const plGameObjectHandle& hChild, plGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Detaches the given child objects from this object and makes them top-level objects.
  void DetachChildren(const plArrayPtr<const plGameObjectHandle>& children, plGameObject::TransformPreservation preserve = TransformPreservation::PreserveGlobal);

  /// \brief Returns the number of children.
  plUInt32 GetChildCount() const;

  /// \brief Returns an iterator over all children of this object.
  ChildIterator GetChildren();

  /// \brief Returns an iterator over all children of this object.
  ConstChildIterator GetChildren() const;

  /// \brief Searches for a child object with the given name. Optionally traverses the entire hierarchy.
  plGameObject* FindChildByName(const plTempHashedString& sName, bool bRecursive = true);

  /// \brief Searches for a child using a path. Every path segment represents a child with a given name.
  ///
  /// Paths are separated with single slashes: /
  /// When an empty path is given, 'this' is returned.
  /// When on any part of the path the next child cannot be found, nullptr is returned.
  /// This function expects an exact path to the destination. It does not search the full hierarchy for
  /// the next child, as SearchChildByNameSequence() does.
  plGameObject* FindChildByPath(plStringView sPath);

  /// \brief Searches for a child similar to FindChildByName() but allows to search for multiple names in a sequence.
  ///
  /// The names in the sequence are separated with slashes.
  /// For example, calling this with "a/b" will first search the entire hierarchy below this object for a child
  /// named "a". If that is found, the search continues from there for a child called "b".
  /// If such a child is found and pExpectedComponent != nullptr, it is verified that the object
  /// contains a component of that type. If it doesn't the search continues (including back-tracking).
  plGameObject* SearchForChildByNameSequence(plStringView sObjectSequence, const plRTTI* pExpectedComponent = nullptr);

  /// \brief Same as SearchForChildByNameSequence but returns ALL matches, in case the given path could mean multiple objects
  void SearchForChildrenByNameSequence(plStringView sObjectSequence, const plRTTI* pExpectedComponent, plHybridArray<plGameObject*, 8>& out_objects);

  plWorld* GetWorld();
  const plWorld* GetWorld() const;


  /// \brief Defines update behavior for global transforms when changing the local transform on a static game object
  enum class UpdateBehaviorIfStatic
  {
    None,              ///< Only sets the local transform, does not update
    UpdateImmediately, ///< Updates the hierarchy underneath the object immediately
  };

  /// \brief Changes the position of the object local to its parent.
  /// \note The rotation of the object itself does not affect the final global position!
  /// The local position is always in the space of the parent object. If there is no parent, local position and global position are
  /// identical.
  void SetLocalPosition(plVec3 vPosition);
  plVec3 GetLocalPosition() const;

  void SetLocalRotation(plQuat qRotation);
  plQuat GetLocalRotation() const;

  void SetLocalScaling(plVec3 vScaling);
  plVec3 GetLocalScaling() const;

  void SetLocalUniformScaling(float fScaling);
  float GetLocalUniformScaling() const;

  plTransform GetLocalTransform() const;

  void SetGlobalPosition(const plVec3& vPosition);
  plVec3 GetGlobalPosition() const;

  void SetGlobalRotation(const plQuat& qRotation);
  plQuat GetGlobalRotation() const;

  void SetGlobalScaling(const plVec3& vScaling);
  plVec3 GetGlobalScaling() const;

  void SetGlobalTransform(const plTransform& transform);
  plTransform GetGlobalTransform() const;

  /// \brief Last frame's global transform (only valid if PL_GAMEOBJECT_VELOCITY is set, otherwise the same as GetGlobalTransform())
  plTransform GetLastGlobalTransform() const;

  // Simd variants of above methods
  void SetLocalPosition(const plSimdVec4f& vPosition, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const plSimdVec4f& GetLocalPositionSimd() const;

  void SetLocalRotation(const plSimdQuat& qRotation, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const plSimdQuat& GetLocalRotationSimd() const;

  void SetLocalScaling(const plSimdVec4f& vScaling, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  const plSimdVec4f& GetLocalScalingSimd() const;

  void SetLocalUniformScaling(const plSimdFloat& fScaling, UpdateBehaviorIfStatic updateBehavior = UpdateBehaviorIfStatic::UpdateImmediately);
  plSimdFloat GetLocalUniformScalingSimd() const;

  plSimdTransform GetLocalTransformSimd() const;

  void SetGlobalPosition(const plSimdVec4f& vPosition);
  const plSimdVec4f& GetGlobalPositionSimd() const;

  void SetGlobalRotation(const plSimdQuat& qRotation);
  const plSimdQuat& GetGlobalRotationSimd() const;

  void SetGlobalScaling(const plSimdVec4f& vScaling);
  const plSimdVec4f& GetGlobalScalingSimd() const;

  void SetGlobalTransform(const plSimdTransform& transform);
  const plSimdTransform& GetGlobalTransformSimd() const;

  const plSimdTransform& GetLastGlobalTransformSimd() const;

  /// \brief Returns the 'forwards' direction of the world's plCoordinateSystem, rotated into the object's global space
  plVec3 GetGlobalDirForwards() const;
  /// \brief Returns the 'right' direction of the world's plCoordinateSystem, rotated into the object's global space
  plVec3 GetGlobalDirRight() const;
  /// \brief Returns the 'up' direction of the world's plCoordinateSystem, rotated into the object's global space
  plVec3 GetGlobalDirUp() const;

#if PL_ENABLED(PL_GAMEOBJECT_VELOCITY)
  /// \brief The last global transform is used to calculate the object's velocity. By default this is set automatically to the global transform of the last frame.
  ///
  /// It might make sense to manually override the last global transform to e.g. indicate an object has been teleported instead of moved.
  void SetLastGlobalTransform(const plSimdTransform& transform);

  /// \brief Returns the linear velocity of the object in units per second. This is only guaranteed to be correct in the PostTransform phase.
  plVec3 GetLinearVelocity() const;

  /// \brief Returns the angular velocity of the object in radians per second. This is only guaranteed to be correct in the PostTransform phase.
  plVec3 GetAngularVelocity() const;
#endif

  /// \brief Updates the global transform immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransform();

  /// \brief Enables or disabled notification message 'plMsgTransformChanged' when this object is static and its transform changes.
  /// The notification message is sent to this object and thus also to all its components.
  void EnableStaticTransformChangesNotifications();
  void DisableStaticTransformChangesNotifications();


  plBoundingBoxSphere GetLocalBounds() const;
  plBoundingBoxSphere GetGlobalBounds() const;

  const plSimdBBoxSphere& GetLocalBoundsSimd() const;
  const plSimdBBoxSphere& GetGlobalBoundsSimd() const;

  /// \brief Invalidates the local bounds and sends a message to all components so they can add their bounds.
  void UpdateLocalBounds();

  /// \brief Updates the global bounds immediately. Usually this done during the world update after the "Post-async" phase.
  /// Note that this function does not ensure that the global transform is up-to-date. Use UpdateGlobalTransformAndBounds if you want to update both.
  void UpdateGlobalBounds();

  /// \brief Updates the global transform and bounds immediately. Usually this done during the world update after the "Post-async" phase.
  void UpdateGlobalTransformAndBounds();


  /// \brief Returns a handle to the internal spatial data.
  plSpatialDataHandle GetSpatialData() const;

  /// \brief Enables or disabled notification message 'plMsgComponentsChanged' when components are added or removed. The message is sent to this object and all its parent objects.
  void EnableComponentChangesNotifications();
  void DisableComponentChangesNotifications();

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  bool TryGetComponentOfBaseType(T*& out_pComponent);

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  template <typename T>
  bool TryGetComponentOfBaseType(const T*& out_pComponent) const;

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  bool TryGetComponentOfBaseType(const plRTTI* pType, plComponent*& out_pComponent);

  /// \brief Tries to find a component of the given base type in the objects components list and returns the first match.
  bool TryGetComponentOfBaseType(const plRTTI* pType, const plComponent*& out_pComponent) const;

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(plDynamicArray<T*>& out_components);

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  template <typename T>
  void TryGetComponentsOfBaseType(plDynamicArray<const T*>& out_components) const;

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const plRTTI* pType, plDynamicArray<plComponent*>& out_components);

  /// \brief Tries to find components of the given base type in the objects components list and returns all matches.
  void TryGetComponentsOfBaseType(const plRTTI* pType, plDynamicArray<const plComponent*>& out_components) const;

  /// \brief Returns a list of all components attached to this object.
  plArrayPtr<plComponent* const> GetComponents();

  /// \brief Returns a list of all components attached to this object.
  plArrayPtr<const plComponent* const> GetComponents() const;

  /// \brief Returns the current version of components attached to this object.
  /// This version is increased whenever components are added or removed and can be used for cache validation.
  plUInt16 GetComponentVersion() const;


  /// \brief Sends a message to all components of this object.
  bool SendMessage(plMessage& ref_msg);

  /// \brief Sends a message to all components of this object.
  bool SendMessage(plMessage& ref_msg) const;

  /// \brief Sends a message to all components of this object and then recursively to all children.
  bool SendMessageRecursive(plMessage& ref_msg);

  /// \brief Sends a message to all components of this object and then recursively to all children.
  bool SendMessageRecursive(plMessage& ref_msg) const;


  /// \brief Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessage(const plMessage& msg, plTime delay, plObjectMsgQueueType::Enum queueType = plObjectMsgQueueType::NextFrame) const;

  /// \brief Queues the message for the given phase. The message is processed after the given delay in the corresponding phase.
  void PostMessageRecursive(const plMessage& msg, plTime delay, plObjectMsgQueueType::Enum queueType = plObjectMsgQueueType::NextFrame) const;

  /// \brief Delivers an plEventMessage to the closest (parent) object containing an plEventMessageHandlerComponent.
  ///
  /// Regular SendMessage() and PostMessage() send a message directly to the target object (and all attached components).
  /// SendMessageRecursive() and PostMessageRecursive() send a message 'down' the graph to the target object and all children.
  ///
  /// In contrast, SendEventMessage() / PostEventMessage() bubble the message 'up' the graph.
  /// They do so by inspecting the chain of parent objects for the existence of an plEventMessageHandlerComponent
  /// (typically a script component). If such a component is found, the message is delivered to it directly, and no other component.
  /// If it is found, but does not handle this type of message, the message is discarded and NOT tried to be delivered
  /// to anyone else.
  ///
  /// If no such component is found in all parent objects, the message is delivered to one plEventMessageHandlerComponent
  /// instances that is set to 'handle global events' (typically used for level-logic scripts), no matter where in the graph it resides.
  /// If multiple global event handler component exist that handle the same message type, the result is non-deterministic.
  ///
  /// \param msg The message to deliver.
  /// \param senderComponent The component that triggered the event in the first place. May be nullptr.
  ///        If not null, this information is stored in \a msg as plEventMessage::m_hSenderObject and plEventMessage::m_hSenderComponent.
  ///        This information is used to pass through more contextual information for the event handler.
  ///        For instance, a trigger component would pass through itself.
  ///        A projectile component sending a 'take damage event' to the hit object, would also pass through itself (the projectile)
  ///        such that the handling code can detect which object was responsible for the damage (and using the plGameObject's team-ID,
  ///        it can detect which player fired the projectile).
  bool SendEventMessage(plMessage& ref_msg, const plComponent* pSenderComponent);

  /// \copydoc plGameObject::SendEventMessage()
  bool SendEventMessage(plMessage& ref_msg, const plComponent* pSenderComponent) const;

  /// \copydoc plGameObject::SendEventMessage()
  ///
  /// \param queueType In which update phase to deliver the message.
  /// \param delay An optional delay before delivering the message.
  void PostEventMessage(plMessage& ref_msg, const plComponent* pSenderComponent, plTime delay, plObjectMsgQueueType::Enum queueType = plObjectMsgQueueType::NextFrame) const;


  /// \brief Returns the tag set associated with this object.
  const plTagSet& GetTags() const;

  /// \brief Sets the tag set associated with this object.
  void SetTags(const plTagSet& tags);

  /// \brief Adds the given tag to the object's tags.
  void SetTag(const plTag& tag);

  /// \brief Removes the given tag from the object's tags.
  void RemoveTag(const plTag& tag);

  /// \brief Returns the 'team ID' that was given during creation (/see plGameObjectDesc)
  ///
  /// It is automatically passed on to objects created by this object.
  /// This makes it possible to identify which player or team an object belongs to.
  const plUInt16& GetTeamID() const { return m_uiTeamID; }

  /// \brief Changes the team ID for this object and all children recursively.
  void SetTeamID(plUInt16 uiId);

  /// \brief Returns a random value that is chosen once during object creation and remains stable even throughout serialization.
  ///
  /// This value is intended to be used for choosing random variations of components. For instance, if a component has two
  /// different meshes it can use for variation, this seed should be used to decide which one to use.
  ///
  /// The stable random seed can also be set from the outside, which is what the editor does, to assign a truly stable seed value.
  /// Therefore, each object placed in the editor will always have the same seed value, and objects won't change their appearance
  /// on every run of the game.
  ///
  /// The stable seed is also propagated through prefab instances, such that every prefab instance gets a different value, but
  /// in a deterministic fashion.
  plUInt32 GetStableRandomSeed() const;

  /// \brief Overwrites the object's random seed value.
  ///
  /// See \a GetStableRandomSeed() for details.
  ///
  /// It should not be necessary to manually change this value, unless you want to make the seed deterministic according to a custom rule.
  void SetStableRandomSeed(plUInt32 uiSeed);

  /// \brief Retrieves a state describing how visible the object is.
  ///
  /// An object may be invisible, fully visible, or indirectly visible (through shadows or reflections).
  /// This can be used to adjust the update logic of objects.
  /// An invisible object may stop updating entirely. An indirectly visible object may reduce its update rate.
  ///
  /// \param uiNumFramesBeforeInvisible Used to treat an object that was visible and just became invisible as visible for a few more frames.
  plVisibilityState GetVisibilityState(plUInt32 uiNumFramesBeforeInvisible = 5) const;

private:
  friend class plComponentManagerBase;
  friend class plGameObjectTest;

  // only needed until reflection can deal with plStringView
  void SetNameInternal(const char* szName);
  const char* GetNameInternal() const;
  void SetGlobalKeyInternal(const char* szKey);
  const char* GetGlobalKeyInternal() const;

  bool SendMessageInternal(plMessage& msg, bool bWasPostedMsg);
  bool SendMessageInternal(plMessage& msg, bool bWasPostedMsg) const;
  bool SendMessageRecursiveInternal(plMessage& msg, bool bWasPostedMsg);
  bool SendMessageRecursiveInternal(plMessage& msg, bool bWasPostedMsg) const;

  PL_ALLOW_PRIVATE_PROPERTIES(plGameObject);

  // Add / Detach child used by the reflected property keep their local transform as
  // updating that is handled by the editor.
  void Reflection_AddChild(plGameObject* pChild);
  void Reflection_DetachChild(plGameObject* pChild);
  plHybridArray<plGameObject*, 8> Reflection_GetChildren() const;
  void Reflection_AddComponent(plComponent* pComponent);
  void Reflection_RemoveComponent(plComponent* pComponent);
  plHybridArray<plComponent*, NUM_INPLACE_COMPONENTS> Reflection_GetComponents() const;

  plObjectMode::Enum Reflection_GetMode() const;
  void Reflection_SetMode(plObjectMode::Enum mode);

  plGameObject* Reflection_GetParent() const;
  void Reflection_SetGlobalPosition(const plVec3& vPosition);
  void Reflection_SetGlobalRotation(const plQuat& qRotation);
  void Reflection_SetGlobalScaling(const plVec3& vScaling);
  void Reflection_SetGlobalTransform(const plTransform& transform);

  bool DetermineDynamicMode(plComponent* pComponentToIgnore = nullptr) const;
  void ConditionalMakeStatic(plComponent* pComponentToIgnore = nullptr);
  void MakeStaticInternal();

  void UpdateGlobalTransformAndBoundsRecursive();
  void UpdateLastGlobalTransform();

  void OnMsgDeleteGameObject(plMsgDeleteGameObject& msg);

  void AddComponent(plComponent* pComponent);
  void RemoveComponent(plComponent* pComponent);
  void FixComponentPointer(plComponent* pOldPtr, plComponent* pNewPtr);

  // Updates the active state of this object and all children and attached components recursively, depending on the enabled states.
  void UpdateActiveState(bool bParentActive);

  void SendNotificationMessage(plMessage& msg);

  struct PL_CORE_DLL alignas(16) TransformationData
  {
    PL_DECLARE_POD_TYPE();

    plGameObject* m_pObject;
    TransformationData* m_pParentData;

#if PL_ENABLED(PL_PLATFORM_32BIT)
    plUInt64 m_uiPadding;
#endif

    plSimdVec4f m_localPosition;
    plSimdQuat m_localRotation;
    plSimdVec4f m_localScaling; // x,y,z = non-uniform scaling, w = uniform scaling

    plSimdTransform m_globalTransform;

#if PL_ENABLED(PL_GAMEOBJECT_VELOCITY)
    plSimdTransform m_lastGlobalTransform;
#endif

    plSimdBBoxSphere m_localBounds; // m_BoxHalfExtents.w != 0 indicates that the object should be always visible
    plSimdBBoxSphere m_globalBounds;

    plSpatialDataHandle m_hSpatialData;
    plUInt32 m_uiSpatialDataCategoryBitmask;

    plUInt32 m_uiStableRandomSeed = 0;

#if PL_ENABLED(PL_GAMEOBJECT_VELOCITY)
    plUInt32 m_uiLastGlobalTransformUpdateCounter = 0;
#else
    plUInt32 m_uiPadding2[1];
#endif

    /// \brief Recomputes the local transform from this object's global transform and, if available, the parent's global transform.
    void UpdateLocalTransform();

    /// \brief Calls UpdateGlobalTransformWithoutParent or UpdateGlobalTransformWithParent depending on whether there is a parent transform.
    /// In case there is a parent transform it also recursively calls itself on the parent transform to ensure everything is up-to-date.
    void UpdateGlobalTransformRecursive(plUInt32 uiUpdateCounter);

    /// \brief Calls UpdateGlobalTransformWithoutParent or UpdateGlobalTransformWithParent depending on whether there is a parent transform.
    /// Assumes that the parent's global transform is already up to date.
    void UpdateGlobalTransformNonRecursive(plUInt32 uiUpdateCounter);

    /// \brief Updates the global transform by copying the object's local transform into the global transform.
    /// This is for objects that have no parent.
    void UpdateGlobalTransformWithoutParent(plUInt32 uiUpdateCounter);

    /// \brief Updates the global transform by combining the parents global transform with this object's local transform.
    /// Assumes that the parent's global transform is already up to date.
    void UpdateGlobalTransformWithParent(plUInt32 uiUpdateCounter);

    void UpdateGlobalBounds(plSpatialSystem* pSpatialSystem);
    void UpdateGlobalBounds();
    void UpdateGlobalBoundsAndSpatialData(plSpatialSystem& ref_spatialSystem);

    void UpdateLastGlobalTransform(plUInt32 uiUpdateCounter);

    void RecreateSpatialData(plSpatialSystem& ref_spatialSystem);
  };

  plGameObjectId m_InternalId;
  plHashedString m_sName;

#if PL_ENABLED(PL_PLATFORM_32BIT)
  plUInt32 m_uiNamePadding;
#endif

  plBitflags<plObjectFlags> m_Flags;

  plUInt32 m_uiParentIndex = 0;
  plUInt32 m_uiFirstChildIndex = 0;
  plUInt32 m_uiLastChildIndex = 0;

  plUInt32 m_uiNextSiblingIndex = 0;
  plUInt32 m_uiPrevSiblingIndex = 0;
  plUInt32 m_uiChildCount = 0;

  plUInt16 m_uiHierarchyLevel = 0;

  /// An int that will be passed on to objects spawned from this one, which allows to identify which team or player it belongs to.
  plUInt16 m_uiTeamID = 0;

  TransformationData* m_pTransformationData = nullptr;

#if PL_ENABLED(PL_PLATFORM_32BIT)
  plUInt32 m_uiPadding = 0;
#endif

  plSmallArrayBase<plComponent*, NUM_INPLACE_COMPONENTS> m_Components;

  struct ComponentUserData
  {
    plUInt16 m_uiVersion;
    plUInt16 m_uiUnused;
  };

  plTagSet m_Tags;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plGameObject);

#include <Core/World/Implementation/GameObject_inl.h>
