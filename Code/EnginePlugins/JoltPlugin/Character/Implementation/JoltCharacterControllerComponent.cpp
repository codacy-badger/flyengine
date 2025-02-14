#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Physics/SurfaceResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Utilities/Stats.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_BITFLAGS(plJoltCharacterDebugFlags, 1)
PL_BITFLAGS_CONSTANTS(plJoltCharacterDebugFlags::PrintState, plJoltCharacterDebugFlags::VisShape, plJoltCharacterDebugFlags::VisContacts,  plJoltCharacterDebugFlags::VisCasts, plJoltCharacterDebugFlags::VisGroundContact, plJoltCharacterDebugFlags::VisFootCheck)
PL_END_STATIC_REFLECTED_BITFLAGS;

PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plJoltCharacterControllerComponent, 1)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PL_MEMBER_PROPERTY("PresenceCollisionLayer", m_uiPresenceCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PL_ACCESSOR_PROPERTY("Mass", GetMass, SetMass)->AddAttributes(new plDefaultValueAttribute(70.0f), new plClampValueAttribute(0.1f, 10000.0f)),
    PL_ACCESSOR_PROPERTY("Strength", GetStrength, SetStrength)->AddAttributes(new plDefaultValueAttribute(500.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("MaxClimbingSlope", GetMaxClimbingSlope, SetMaxClimbingSlope)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(40))),
    PL_BITFLAGS_MEMBER_PROPERTY("DebugFlags", plJoltCharacterDebugFlags , m_DebugFlags),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Physics/Jolt/Character"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

plJoltCharacterControllerComponent::plJoltCharacterControllerComponent() = default;
plJoltCharacterControllerComponent::~plJoltCharacterControllerComponent() = default;

void plJoltCharacterControllerComponent::SetObjectToIgnore(plUInt32 uiObjectFilterID)
{
  m_BodyFilter.m_uiObjectFilterIDToIgnore = uiObjectFilterID;
}

void plJoltCharacterControllerComponent::ClearObjectToIgnore()
{
  m_BodyFilter.ClearFilter();
}

void plJoltCharacterControllerComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_DebugFlags;

  s << m_uiCollisionLayer;
  s << m_uiPresenceCollisionLayer;
  s << m_fMass;
  s << m_fStrength;
  s << m_MaxClimbingSlope;
}

void plJoltCharacterControllerComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_DebugFlags;

  s >> m_uiCollisionLayer;
  s >> m_uiPresenceCollisionLayer;
  s >> m_fMass;
  s >> m_fStrength;
  s >> m_MaxClimbingSlope;
}

void plJoltCharacterControllerComponent::OnDeactivated()
{
  if (m_pCharacter)
  {
    if (plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>())
    {
      pModule->ActivateCharacterController(this, false);
    }

    m_pCharacter->Release();
    m_pCharacter = nullptr;
  }

  RemovePresenceBody();

  SUPER::OnDeactivated();
}

void plJoltCharacterControllerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

  JPH::CharacterVirtualSettings opt;
  opt.mUp = JPH::Vec3::sAxisZ();
  opt.mSupportingVolume = JPH::Plane(opt.mUp, -GetShapeRadius());
  opt.mShape = MakeNextCharacterShape();
  opt.mMaxSlopeAngle = m_MaxClimbingSlope.GetRadian();
  opt.mMass = m_fMass;
  opt.mMaxStrength = m_fStrength;

  const plTransform ownTrans = GetOwner()->GetGlobalTransform();

  m_pCharacter = new JPH::CharacterVirtual(&opt, plJoltConversionUtils::ToVec3(ownTrans.m_vPosition), plJoltConversionUtils::ToQuat(ownTrans.m_qRotation), pModule->GetJoltSystem());
  m_pCharacter->AddRef();

  pModule->ActivateCharacterController(this, true);

  CreatePresenceBody();
}

void plJoltCharacterControllerComponent::SetMaxClimbingSlope(plAngle slope)
{
  m_MaxClimbingSlope = slope;

  if (m_pCharacter)
  {
    m_pCharacter->SetMaxSlopeAngle(m_MaxClimbingSlope.GetRadian());
  }
}

void plJoltCharacterControllerComponent::SetMass(float fMass)
{
  m_fMass = fMass;

  if (m_pCharacter)
  {
    m_pCharacter->SetMass(m_fMass);
  }
}

void plJoltCharacterControllerComponent::SetStrength(float fStrength)
{
  m_fStrength = fStrength;

  if (m_pCharacter)
  {
    m_pCharacter->SetMaxStrength(m_fStrength);
  }
}

plResult plJoltCharacterControllerComponent::TryChangeShape(JPH::Shape* pNewShape)
{
  plJoltBroadPhaseLayerFilter broadphaseFilter(plPhysicsShapeType::Static | plPhysicsShapeType::Dynamic);
  plJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);

  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();

  if (m_pCharacter->SetShape(pNewShape, 0.01f, broadphaseFilter, objectFilter, m_BodyFilter, {}, *pModule->GetTempAllocator()))
  {
    RemovePresenceBody();
    CreatePresenceBody();

    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

void plJoltCharacterControllerComponent::RawMoveWithVelocity(const plVec3& vVelocity, float fMaxStairStepUp, float fMaxStepDown)
{
  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();

  plJoltBroadPhaseLayerFilter broadphaseFilter(plPhysicsShapeType::Static | plPhysicsShapeType::Dynamic);
  plJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);

  m_pCharacter->SetLinearVelocity(plJoltConversionUtils::ToVec3(vVelocity));

  // Settings for our update function
  JPH::CharacterVirtual::ExtendedUpdateSettings updateSettings;
  updateSettings.mStickToFloorStepDown = JPH::Vec3(0, 0, -fMaxStepDown);
  updateSettings.mWalkStairsStepUp = fMaxStairStepUp > 0 ? JPH::Vec3(0, 0, fMaxStairStepUp) : JPH::Vec3::sZero();

  // Update the character position
  m_pCharacter->ExtendedUpdate(GetUpdateTimeDelta(), plJoltConversionUtils::ToVec3(pModule->GetCharacterGravity()), updateSettings, broadphaseFilter, objectFilter, m_BodyFilter, {}, *pModule->GetTempAllocator());

  GetOwner()->SetGlobalPosition(plJoltConversionUtils::ToSimdVec3(m_pCharacter->GetPosition()));
}


void plJoltCharacterControllerComponent::RawMoveIntoDirection(const plVec3& vDirection)
{
  if (vDirection.IsZero())
    return;

  RawMoveWithVelocity(vDirection * GetInverseUpdateTimeDelta(), 0.0f, 0.0f);
}

void plJoltCharacterControllerComponent::RawMoveToPosition(const plVec3& vTargetPosition)
{
  RawMoveIntoDirection(vTargetPosition - GetOwner()->GetGlobalPosition());
}

void plJoltCharacterControllerComponent::TeleportToPosition(const plVec3& vGlobalFootPos)
{
  m_pCharacter->SetPosition(plJoltConversionUtils::ToVec3(vGlobalFootPos));

  plJoltBroadPhaseLayerFilter broadphaseFilter(plPhysicsShapeType::Static | plPhysicsShapeType::Dynamic);
  plJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);

  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();

  m_pCharacter->RefreshContacts(broadphaseFilter, objectFilter, m_BodyFilter, {}, *pModule->GetTempAllocator());
}

bool plJoltCharacterControllerComponent::StickToGround(float fMaxDist)
{
  if (m_pCharacter->GetGroundState() != JPH::CharacterBase::EGroundState::InAir || m_pCharacter->GetGroundState() != JPH::CharacterBase::EGroundState::NotSupported)
    return false;

  plJoltBroadPhaseLayerFilter broadphaseFilter(plPhysicsShapeType::Static | plPhysicsShapeType::Dynamic);
  plJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);

  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();

  return m_pCharacter->StickToFloor(JPH::Vec3(0, 0, -fMaxDist), broadphaseFilter, objectFilter, m_BodyFilter, {}, *pModule->GetTempAllocator());
}

void plJoltCharacterControllerComponent::CollectCastContacts(plDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const plVec3& vQueryPosition, const plQuat& qQueryRotation, const plVec3& vSweepDir) const
{
  out_Contacts.Clear();

  class ContactCastCollector : public JPH::CastShapeCollector
  {
  public:
    plDynamicArray<ContactPoint>* m_pContacts = nullptr;
    const JPH::BodyLockInterface* m_pLockInterface = nullptr;

    virtual void AddHit(const JPH::ShapeCastResult& result) override
    {
      auto& contact = m_pContacts->ExpandAndGetRef();
      contact.m_vPosition = plJoltConversionUtils::ToVec3(result.mContactPointOn2);
      contact.m_vContactNormal = plJoltConversionUtils::ToVec3(-result.mPenetrationAxis.Normalized());
      contact.m_BodyID = result.mBodyID2;
      contact.m_fCastFraction = result.mFraction;
      contact.m_SubShapeID = result.mSubShapeID2;

      JPH::BodyLockRead lock(*m_pLockInterface, contact.m_BodyID);
      contact.m_vSurfaceNormal = plJoltConversionUtils::ToVec3(lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, result.mContactPointOn2));
    }
  };

  const plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();
  const auto pJoltSystem = pModule->GetJoltSystem();

  plJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);
  plJoltBroadPhaseLayerFilter broadphaseFilter(plPhysicsShapeType::Static | plPhysicsShapeType::Dynamic);

  ContactCastCollector collector;
  collector.m_pLockInterface = &pJoltSystem->GetBodyLockInterfaceNoLock();
  collector.m_pContacts = &out_Contacts;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(plJoltConversionUtils::ToQuat(qQueryRotation), plJoltConversionUtils::ToVec3(vQueryPosition));

  JPH::RShapeCast castOpt(pShape, JPH::Vec3::sReplicate(1.0f), trans, plJoltConversionUtils::ToVec3(vSweepDir));

  JPH::ShapeCastSettings settings;
  pJoltSystem->GetNarrowPhaseQuery().CastShape(castOpt, settings, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, m_BodyFilter);
}

void plJoltCharacterControllerComponent::CollectContacts(plDynamicArray<ContactPoint>& out_Contacts, const JPH::Shape* pShape, const plVec3& vQueryPosition, const plQuat& qQueryRotation, float fCollisionTolerance) const
{
  out_Contacts.Clear();

  class ContactCollector : public JPH::CollideShapeCollector
  {
  public:
    plDynamicArray<ContactPoint>* m_pContacts = nullptr;
    const JPH::BodyLockInterface* m_pLockInterface = nullptr;

    virtual void AddHit(const JPH::CollideShapeResult& result) override
    {
      auto& contact = m_pContacts->ExpandAndGetRef();
      contact.m_vPosition = plJoltConversionUtils::ToVec3(result.mContactPointOn2);
      contact.m_vContactNormal = plJoltConversionUtils::ToVec3(-result.mPenetrationAxis.Normalized());
      contact.m_BodyID = result.mBodyID2;
      contact.m_SubShapeID = result.mSubShapeID2;

      JPH::BodyLockRead lock(*m_pLockInterface, contact.m_BodyID);
      contact.m_vSurfaceNormal = plJoltConversionUtils::ToVec3(lock.GetBody().GetWorldSpaceSurfaceNormal(result.mSubShapeID2, result.mContactPointOn2));
    }
  };

  const plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();
  const auto pJoltSystem = pModule->GetJoltSystem();

  plJoltObjectLayerFilter objectFilter(m_uiCollisionLayer);
  plJoltBroadPhaseLayerFilter broadphaseFilter(plPhysicsShapeType::Static | plPhysicsShapeType::Dynamic);

  ContactCollector collector;
  collector.m_pLockInterface = &pJoltSystem->GetBodyLockInterfaceNoLock();
  collector.m_pContacts = &out_Contacts;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(plJoltConversionUtils::ToQuat(qQueryRotation), plJoltConversionUtils::ToVec3(vQueryPosition));

  JPH::CollideShapeSettings settings;
  settings.mCollisionTolerance = fCollisionTolerance;
  settings.mBackFaceMode = JPH::EBackFaceMode::CollideWithBackFaces;

  pJoltSystem->GetNarrowPhaseQuery().CollideShape(pShape, JPH::Vec3::sReplicate(1.0f), trans, settings, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, m_BodyFilter);
}

plVec3 plJoltCharacterControllerComponent::GetContactVelocityAndPushAway(const ContactPoint& contact, float fPushForce)
{
  if (contact.m_BodyID.IsInvalid())
    return plVec3::MakeZero();

  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();
  auto pJoltSystem = pModule->GetJoltSystem();

  JPH::BodyLockWrite bodyLock(pJoltSystem->GetBodyLockInterface(), contact.m_BodyID);

  if (!bodyLock.Succeeded())
    return plVec3::MakeZero();

  const JPH::Vec3 vGroundPos = plJoltConversionUtils::ToVec3(contact.m_vPosition);

  if (fPushForce > 0 && bodyLock.GetBody().IsDynamic())
  {
    const plVec3 vPushDir = -contact.m_vSurfaceNormal * fPushForce;

    bodyLock.GetBody().AddForce(plJoltConversionUtils::ToVec3(vPushDir), vGroundPos);
    pJoltSystem->GetBodyInterfaceNoLock().ActivateBody(contact.m_BodyID);
  }

  plVec3 vGroundVelocity = plVec3::MakeZero();

  if (bodyLock.GetBody().IsKinematic())
  {
    vGroundVelocity = plJoltConversionUtils::ToVec3(bodyLock.GetBody().GetPointVelocity(vGroundPos));
    vGroundVelocity.z = 0;
  }

  return vGroundVelocity;
}

void plJoltCharacterControllerComponent::SpawnContactInteraction(const ContactPoint& contact, const plHashedString& sSurfaceInteraction, plSurfaceResourceHandle hFallbackSurface, const plVec3& vInteractionNormal)
{
  if (contact.m_BodyID.IsInvalid())
    return;

  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();

  plSurfaceResourceHandle hSurface = hFallbackSurface;

  JPH::BodyLockRead lock(pModule->GetJoltSystem()->GetBodyLockInterfaceNoLock(), contact.m_BodyID);
  if (lock.Succeeded())
  {
    auto pMat = static_cast<const plJoltMaterial*>(lock.GetBody().GetShape()->GetMaterial(contact.m_SubShapeID));
    if (pMat && pMat->m_pSurface)
    {
      hSurface = static_cast<const plJoltMaterial*>(pMat)->m_pSurface->GetResourceHandle();
    }
  }

  if (hSurface.IsValid())
  {
    plResourceLock<plSurfaceResource> pSurface(hSurface, plResourceAcquireMode::AllowLoadingFallback);
    pSurface->InteractWithSurface(GetWorld(), plGameObjectHandle(), contact.m_vPosition, contact.m_vSurfaceNormal, vInteractionNormal, sSurfaceInteraction, &GetOwner()->GetTeamID());
  }
}

void plJoltCharacterControllerComponent::VisualizeContact(const ContactPoint& contact, const plColor& color) const
{
  plTransform trans;
  trans.m_vPosition = contact.m_vPosition;
  trans.m_qRotation = plQuat::MakeShortestRotation(plVec3::MakeAxisX(), contact.m_vContactNormal);
  trans.m_vScale.Set(1.0f);

  plDebugRenderer::DrawCylinder(GetWorld(), 0, 0.05f, 0.1f, plColor::MakeZero(), color, trans);
}

void plJoltCharacterControllerComponent::VisualizeContacts(const plDynamicArray<ContactPoint>& contacts, const plColor& color) const
{
  for (const auto& ct : contacts)
  {
    VisualizeContact(ct, color);
  }
}

void plJoltCharacterControllerComponent::Update(plTime deltaTime)
{
  m_fUpdateTimeDelta = deltaTime.AsFloatInSeconds();
  m_fInverseUpdateTimeDelta = static_cast<float>(1.0 / deltaTime.GetSeconds());

  UpdateCharacter();

  MovePresenceBody(deltaTime);
}

void plJoltCharacterControllerComponent::CreatePresenceBody()
{
  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  const plSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  JPH::BodyCreationSettings bodyCfg;
  bodyCfg.SetShape(m_pCharacter->GetShape());

  plJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  plUInt32 m_uiObjectFilterID = pModule->CreateObjectFilterID();

  bodyCfg.mPosition = plJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = plJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized();
  bodyCfg.mMotionType = JPH::EMotionType::Kinematic;
  bodyCfg.mObjectLayer = plJoltCollisionFiltering::ConstructObjectLayer(m_uiPresenceCollisionLayer, plJoltBroadphaseLayer::Character);
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter());
  bodyCfg.mUserData = reinterpret_cast<plUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  m_uiPresenceBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  m_uiPresenceBodyAddCounter = pModule->QueueBodyToAdd(pBody, true);
}

void plJoltCharacterControllerComponent::RemovePresenceBody()
{
  if (m_uiPresenceBodyID == plInvalidIndex)
    return;

  JPH::BodyID bodyId(m_uiPresenceBodyID);

  m_uiPresenceBodyID = plInvalidIndex;

  if (bodyId.IsInvalid())
    return;

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  pBodies->RemoveBody(bodyId);
  pBodies->DestroyBody(bodyId);

  pModule->DeallocateUserData(m_uiUserDataIndex);
  // pModule->DeleteObjectFilterID(m_uiObjectFilterID);
}

void plJoltCharacterControllerComponent::MovePresenceBody(plTime deltaTime)
{
  if (m_uiPresenceBodyID == plInvalidIndex)
    return;

  JPH::BodyID bodyId(m_uiPresenceBodyID);

  if (bodyId.IsInvalid())
    return;

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

  if (pModule->IsBodyStillQueuedToAdd(m_uiPresenceBodyAddCounter))
    return;

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  const plSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  const float tDiff = deltaTime.AsFloatInSeconds();

  pBodies->MoveKinematic(bodyId, plJoltConversionUtils::ToVec3(trans.m_Position), plJoltConversionUtils::ToQuat(trans.m_Rotation).Normalized(), tDiff);
}


PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_Character_Implementation_JoltCharacterControllerComponent);
