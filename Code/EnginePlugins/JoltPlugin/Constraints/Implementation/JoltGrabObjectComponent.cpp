#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GrabbableItemComponent.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Character/JoltCharacterControllerComponent.h>
#include <JoltPlugin/Constraints/JoltGrabObjectComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <Physics/Constraints/SixDOFConstraint.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plJoltGrabObjectComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("MaxGrabPointDistance", m_fMaxGrabPointDistance)->AddAttributes(new plDefaultValueAttribute(2.0f)),
    PL_MEMBER_PROPERTY("CastRadius", m_fCastRadius)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PL_MEMBER_PROPERTY("SpringStiffness", m_fSpringStiffness)->AddAttributes(new plDefaultValueAttribute(2.0f), new plClampValueAttribute(1.0f, 60.0f)),
    PL_MEMBER_PROPERTY("SpringDamping", m_fSpringDamping)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, 1.0f)),
    PL_MEMBER_PROPERTY("BreakDistance", m_fBreakDistance)->AddAttributes(new plDefaultValueAttribute(0.5f)),
    PL_ACCESSOR_PROPERTY("AttachTo", DummyGetter, SetAttachToReference)->AddAttributes(new plGameObjectReferenceAttribute()),
    PL_MEMBER_PROPERTY("GrabAnyObjectWithSize", m_fAllowGrabAnyObjectWithSize)->AddAttributes(new plDefaultValueAttribute(0.75f)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(GrabNearbyObject),
    PL_SCRIPT_FUNCTION_PROPERTY(HasObjectGrabbed),
    PL_SCRIPT_FUNCTION_PROPERTY(DropGrabbedObject),
    PL_SCRIPT_FUNCTION_PROPERTY(ThrowGrabbedObject, In, "Direction"),
    PL_SCRIPT_FUNCTION_PROPERTY(BreakObjectGrab),
  }
  PL_END_FUNCTIONS;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgReleaseObjectGrab, OnMsgReleaseObjectGrab),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Physics/Jolt/Constraints"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plJoltGrabObjectComponent::plJoltGrabObjectComponent() = default;
plJoltGrabObjectComponent::~plJoltGrabObjectComponent() = default;

void plJoltGrabObjectComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fBreakDistance;
  s << m_fSpringStiffness;
  s << m_fSpringDamping;
  s << m_fMaxGrabPointDistance;
  s << m_fCastRadius;
  s << m_uiCollisionLayer;
  s << m_fAllowGrabAnyObjectWithSize;

  inout_stream.WriteGameObjectHandle(m_hAttachTo);
}

void plJoltGrabObjectComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fBreakDistance;
  s >> m_fSpringStiffness;
  s >> m_fSpringDamping;
  s >> m_fMaxGrabPointDistance;
  if (uiVersion >= 2)
  {
    s >> m_fCastRadius;
  }
  s >> m_uiCollisionLayer;
  s >> m_fAllowGrabAnyObjectWithSize;

  m_hAttachTo = inout_stream.ReadGameObjectHandle();
}

bool plJoltGrabObjectComponent::FindNearbyObject(plGameObject*& out_pObject, plTransform& out_localGrabPoint) const
{
  const plPhysicsWorldModuleInterface* pPhysicsModule = GetWorld()->GetModuleReadOnly<plPhysicsWorldModuleInterface>();

  if (pPhysicsModule == nullptr)
    return false;

  auto pOwner = GetOwner();

  plPhysicsCastResult hit;
  plPhysicsQueryParameters queryParam;
  queryParam.m_bIgnoreInitialOverlap = true;
  queryParam.m_uiCollisionLayer = m_uiCollisionLayer;
  queryParam.m_ShapeTypes = plPhysicsShapeType::Static | plPhysicsShapeType::Dynamic;

  if (m_fCastRadius > 0.0f)
  {
    if (!pPhysicsModule->SweepTestSphere(hit, m_fCastRadius, pOwner->GetGlobalPosition(), pOwner->GetGlobalDirForwards().GetNormalized(), m_fMaxGrabPointDistance * 5.0f, queryParam))
      return false;
  }
  else
  {
    if (!pPhysicsModule->Raycast(hit, pOwner->GetGlobalPosition(), pOwner->GetGlobalDirForwards().GetNormalized(), m_fMaxGrabPointDistance * 5.0f, queryParam))
      return false;
  }  

  const plGameObject* pActorObj = nullptr;
  if (!GetWorld()->TryGetObject(hit.m_hActorObject, pActorObj))
    return false;

  // If we hit a non-kinematic dynamic actor try to find a grab point.
  // If we don't have an dynamic actor or it is kinematic still report the hit so it can be used for other interactions like buttons etc.
  const plJoltDynamicActorComponent* pActorComp = nullptr;
  if (pActorObj->TryGetComponentOfBaseType(pActorComp) && !pActorComp->GetKinematic())
  {
    if (DetermineGrabPoint(pActorComp, out_localGrabPoint).Failed())
      return false;
  }
  else
  {
    if (hit.m_fDistance > m_fMaxGrabPointDistance)
      return false;

    out_localGrabPoint = plTransform::MakeIdentity();
  }

  out_pObject = const_cast<plGameObject*>(pActorObj);
  return true;
}

bool plJoltGrabObjectComponent::GrabObject(plGameObject* pObjectToGrab, const plTransform& localGrabPoint)
{
  if (m_pConstraint != nullptr || pObjectToGrab == nullptr)
    return false;

  const plTime curTime = GetWorld()->GetClock().GetAccumulatedTime();

  // a cooldown to grab something again after we stood on the held object
  if (m_LastValidTime > curTime)
    return false;

  plJoltDynamicActorComponent* pAttachToActor = GetAttachToActor();
  if (pAttachToActor == nullptr)
  {
    plLog::Error("Can't grab object, no target actor to attach it to is set.");
    return false;
  }

  plJoltDynamicActorComponent* pActorToGrab = nullptr;
  if (!pObjectToGrab->TryGetComponentOfBaseType(pActorToGrab))
    return false;

  if (pActorToGrab->GetKinematic())
    return false;

  if (IsCharacterStandingOnObject(pObjectToGrab->GetHandle()))
    return false;

  plJoltCharacterControllerComponent* pController;
  if (GetWorld()->TryGetComponent(m_hCharacterControllerComponent, pController))
  {
    pController->SetObjectToIgnore(pActorToGrab->GetObjectFilterID());
  }

  m_ChildAnchorLocal = localGrabPoint;
  m_hGrabbedActor = pActorToGrab->GetHandle();

  CreateJoint(pAttachToActor, pActorToGrab);

  plMsgObjectGrabbed msg;
  msg.m_bGotGrabbed = true;
  msg.m_hGrabbedBy = GetOwner()->GetHandle();
  pActorToGrab->GetOwner()->SendMessage(msg);

  m_LastValidTime = curTime;

  return true;
}

bool plJoltGrabObjectComponent::GrabNearbyObject()
{
  plGameObject* pActorToGrab = nullptr;
  plTransform localGrabPoint;
  if (!FindNearbyObject(pActorToGrab, localGrabPoint))
    return false;

  return GrabObject(pActorToGrab, localGrabPoint);
}

bool plJoltGrabObjectComponent::HasObjectGrabbed() const
{
  return m_pConstraint != nullptr;
}

void plJoltGrabObjectComponent::DropGrabbedObject()
{
  ReleaseGrabbedObject();
}

void plJoltGrabObjectComponent::ThrowGrabbedObject(const plVec3& vRelativeDir)
{
  plComponentHandle hActor = m_hGrabbedActor;
  ReleaseGrabbedObject();

  plJoltDynamicActorComponent* pActor;
  if (GetWorld()->TryGetComponent(hActor, pActor))
  {
    // TODO: normalize impulse with object mass (?)

    pActor->AddLinearImpulse(GetOwner()->GetGlobalRotation() * vRelativeDir);
  }
}

void plJoltGrabObjectComponent::BreakObjectGrab()
{
  ReleaseGrabbedObject();

  plMsgPhysicsJointBroke msg;
  msg.m_hJointObject = GetOwner()->GetHandle();

  GetOwner()->PostEventMessage(msg, this, plTime::MakeZero());
}

void plJoltGrabObjectComponent::SetAttachToReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hAttachTo = resolver(szReference, GetHandle(), "AttachTo");
}

void plJoltGrabObjectComponent::ReleaseGrabbedObject()
{
  if (m_pConstraint == nullptr)
    return;

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

  plJoltDynamicActorComponent* pGrabbedActor = nullptr;
  if (GetWorld()->TryGetComponent(m_hGrabbedActor, pGrabbedActor))
  {
    JPH::BodyLockWrite bodyLock(pModule->GetJoltSystem()->GetBodyLockInterface(), JPH::BodyID(pGrabbedActor->GetJoltBodyID()));
    if (bodyLock.Succeeded())
    {
      bodyLock.GetBody().GetMotionProperties()->SetInverseMass(m_fGrabbedActorInverseMass);
      // TODO: this needs to be set as well : bodyLock.GetBody().GetMotionProperties()->SetInverseInertia(m_fGrabbedActorMass);
      bodyLock.GetBody().GetMotionProperties()->SetGravityFactor(m_fGrabbedActorGravity);

      if (pModule->GetJoltSystem()->GetBodyInterfaceNoLock().IsAdded(JPH::BodyID(pGrabbedActor->GetJoltBodyID())))
      {
        pModule->GetJoltSystem()->GetBodyInterfaceNoLock().ActivateBody(JPH::BodyID(pGrabbedActor->GetJoltBodyID()));
      }
    }

    plMsgObjectGrabbed msg;
    msg.m_bGotGrabbed = false;
    msg.m_hGrabbedBy = GetOwner()->GetHandle();
    pGrabbedActor->GetOwner()->SendMessage(msg);
  }

  plJoltCharacterControllerComponent* pController;
  if (GetWorld()->TryGetComponent(m_hCharacterControllerComponent, pController))
  {
    pController->ClearObjectToIgnore();
  }

  pModule->GetJoltSystem()->RemoveConstraint(m_pConstraint);

  m_pConstraint->Release();
  m_pConstraint = nullptr;

  m_hGrabbedActor.Invalidate();
}

plJoltDynamicActorComponent* plJoltGrabObjectComponent::GetAttachToActor()
{
  plJoltDynamicActorComponent* pActor = nullptr;
  plGameObject* pObject = nullptr;

  if (!GetWorld()->TryGetObject(m_hAttachTo, pObject))
    return nullptr;

  if (!pObject->TryGetComponentOfBaseType(pActor))
    return nullptr;

  if (!pActor->GetKinematic())
    return nullptr;

  return pActor;
}

plResult plJoltGrabObjectComponent::DetermineGrabPoint(const plComponent* pActorComp, plTransform& out_LocalGrabPoint) const
{
  out_LocalGrabPoint.SetIdentity();

  const plGameObject* pAttachToObject = nullptr;
  if (!GetWorld()->TryGetObject(m_hAttachTo, pAttachToObject))
    return PL_FAILURE;

  const auto vAttachToPos = pAttachToObject->GetGlobalPosition();
  const auto vOwnerDir = GetOwner()->GetGlobalDirForwards();
  const auto vOwnerUp = GetOwner()->GetGlobalDirUp();
  const auto pActorObj = pActorComp->GetOwner();

  const plTransform& actorTransform = pActorObj->GetGlobalTransform();
  plHybridArray<plGrabbableItemGrabPoint, 16> grabPoints;

  const plGrabbableItemComponent* pGrabbableItemComp = nullptr;
  if (pActorObj->TryGetComponentOfBaseType(pGrabbableItemComp) && !pGrabbableItemComp->m_GrabPoints.IsEmpty())
  {
    grabPoints = pGrabbableItemComp->m_GrabPoints;
  }
  else
  {
    plBoundingBoxSphere bounds = pActorComp->GetOwner()->GetLocalBounds();

    if (!bounds.IsValid())
    {
      bounds = plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), 0.1f);
    }

    const auto& box = bounds.GetBox();
    const plVec3 ext = box.GetExtents().CompMul(pActorComp->GetOwner()->GetGlobalScaling());

    if (ext.x <= m_fAllowGrabAnyObjectWithSize && ext.y <= m_fAllowGrabAnyObjectWithSize && ext.z <= m_fAllowGrabAnyObjectWithSize)
    {
      const plVec3 halfExt = box.GetHalfExtents().CompMul(pActorComp->GetOwner()->GetGlobalScaling()) * 0.5f;
      const plVec3& center = box.GetCenter();

      grabPoints.SetCount(4);
      grabPoints[0].m_vLocalPosition.Set(-halfExt.x, 0, 0);
      grabPoints[0].m_qLocalRotation = plQuat::MakeShortestRotation(plVec3::MakeAxisX(), plVec3::MakeAxisX());
      grabPoints[1].m_vLocalPosition.Set(+halfExt.x, 0, 0);
      grabPoints[1].m_qLocalRotation = plQuat::MakeShortestRotation(plVec3::MakeAxisX(), -plVec3::MakeAxisX());
      grabPoints[2].m_vLocalPosition.Set(0, -halfExt.y, 0);
      grabPoints[2].m_qLocalRotation = plQuat::MakeShortestRotation(plVec3::MakeAxisX(), plVec3::MakeAxisY());
      grabPoints[3].m_vLocalPosition.Set(0, +halfExt.y, 0);
      grabPoints[3].m_qLocalRotation = plQuat::MakeShortestRotation(plVec3::MakeAxisX(), -plVec3::MakeAxisY());
      // grabPoints[4].m_vLocalPosition.Set(0, 0, -halfExt.z);
      // grabPoints[4].m_qLocalRotation = plQuat::MakeShortestRotation(plVec3::MakeAxisX(), plVec3::MakeAxisZ());
      // grabPoints[5].m_vLocalPosition.Set(0, 0, +halfExt.z);
      // grabPoints[5].m_qLocalRotation = plQuat::MakeShortestRotation(plVec3::MakeAxisX(), -plVec3::MakeAxisZ());

      for (plUInt32 i = 0; i < grabPoints.GetCount(); ++i)
      {
        grabPoints[i].m_vLocalPosition += center;
      }
    }
  }

  if (grabPoints.IsEmpty())
    return PL_FAILURE;

  const float fMaxDistSqr = plMath::Square(m_fMaxGrabPointDistance);

  plUInt32 uiBestPointIndex = plInvalidIndex;
  float fBestScore = -1000.0f;

  for (plUInt32 i = 0; i < grabPoints.GetCount(); ++i)
  {
    const plVec3 vGrabPointPos = actorTransform.TransformPosition(grabPoints[i].m_vLocalPosition);
    const plQuat qGrabPointRot = actorTransform.m_qRotation * grabPoints[i].m_qLocalRotation;
    const plVec3 vGrabPointDir = qGrabPointRot * plVec3(1, 0, 0);
    const plVec3 vGrabPointUp = qGrabPointRot * plVec3(0, 0, 1);

    const float fLenSqr = (vGrabPointPos - vAttachToPos).GetLengthSquared();

    if (fLenSqr >= fMaxDistSqr)
      continue;

    float fScore = 1.0f - fLenSqr;
    fScore += vGrabPointDir.Dot(vOwnerDir);
    fScore += vGrabPointUp.Dot(vOwnerUp) * 0.5f; // up has less weight than forward

    if (fScore > fBestScore)
    {
      uiBestPointIndex = i;
      fBestScore = fScore;
    }
  }

  if (uiBestPointIndex >= grabPoints.GetCount())
    return PL_FAILURE;

  out_LocalGrabPoint.m_vPosition = grabPoints[uiBestPointIndex].m_vLocalPosition;
  out_LocalGrabPoint.m_qRotation = grabPoints[uiBestPointIndex].m_qLocalRotation;

  return PL_SUCCESS;
}

void plJoltGrabObjectComponent::CreateJoint(plJoltDynamicActorComponent* pParent, plJoltDynamicActorComponent* pChild)
{
  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

  JPH::BodyID bodies[2] = {JPH::BodyID(pParent->GetJoltBodyID()), JPH::BodyID(pChild->GetJoltBodyID())};
  JPH::BodyLockMultiWrite bodyLock(pModule->GetJoltSystem()->GetBodyLockInterface(), bodies, 2);

  auto pBody0 = bodyLock.GetBody(0);
  auto pBody1 = bodyLock.GetBody(1);

  m_fGrabbedActorInverseMass = pBody1->GetMotionProperties()->GetInverseMass();
  m_fGrabbedActorGravity = pBody1->GetMotionProperties()->GetGravityFactor();

  pBody1->GetMotionProperties()->SetInverseMass(10.0f);
  pBody1->GetMotionProperties()->SetGravityFactor(0.0f);

  JPH::SixDOFConstraintSettings opt;

  {
    const auto diff0 = pBody0->GetPosition() - pBody0->GetCenterOfMassPosition();
    const auto diff1 = pBody1->GetPosition() - pBody1->GetCenterOfMassPosition();

    const JPH::Quat childRot = plJoltConversionUtils::ToQuat(m_ChildAnchorLocal.m_qRotation);

    opt.mDrawConstraintSize = 0.1f;
    opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
    opt.mPosition1 = diff0;
    opt.mPosition2 = diff1 + plJoltConversionUtils::ToVec3(m_ChildAnchorLocal.m_vPosition);
    opt.mAxisX1 = JPH::Vec3::sAxisX();
    opt.mAxisY1 = JPH::Vec3::sAxisY();
    opt.mAxisX2 = childRot * JPH::Vec3::sAxisX();
    opt.mAxisY2 = childRot * JPH::Vec3::sAxisY();
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationX);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationY);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::TranslationZ);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::RotationX);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::RotationY);
    opt.MakeFreeAxis(JPH::SixDOFConstraintSettings::EAxis::RotationZ);

    for (int i = 0; i < 6; ++i)
    {
      opt.mMotorSettings[JPH::SixDOFConstraintSettings::EAxis::TranslationX + i].mSpringSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
      opt.mMotorSettings[JPH::SixDOFConstraintSettings::EAxis::TranslationX + i].mSpringSettings.mDamping = m_fSpringDamping;
      opt.mMotorSettings[JPH::SixDOFConstraintSettings::EAxis::TranslationX + i].mSpringSettings.mFrequency = m_fSpringStiffness;
    }
  }

  // plTransform tAnchor = m_ChildAnchorLocal;
  // tAnchor.m_vPosition = tAnchor.m_vPosition.CompMul(pChild->GetOwner()->GetGlobalScaling());
  // pJoint->SetActors(pParent->GetOwner()->GetHandle(), plTransform::MakeIdentity(), pChild->GetOwner()->GetHandle(), tAnchor);

  m_pConstraint = static_cast<JPH::SixDOFConstraint*>(opt.Create(*bodyLock.GetBody(0), *bodyLock.GetBody(1)));
  m_pConstraint->AddRef();

  for (int i = 0; i < 6; ++i)
  {
    m_pConstraint->SetMotorState((JPH::SixDOFConstraint::EAxis)(JPH::SixDOFConstraint::EAxis::TranslationX + i), JPH::EMotorState::Position);
  }

  pModule->GetJoltSystem()->AddConstraint(m_pConstraint);
  pModule->GetJoltSystem()->GetBodyInterfaceNoLock().ActivateBodies(bodies, 2);
}

void plJoltGrabObjectComponent::DetectDistanceViolation(plJoltDynamicActorComponent* pGrabbedActor)
{
  if (m_fBreakDistance <= 0)
    return;

  plGameObject* pJointObject = nullptr;
  if (!GetWorld()->TryGetObject(m_hAttachTo, pJointObject))
    return;

  const plVec3 vAnchorPos = pGrabbedActor->GetOwner()->GetGlobalTransform().TransformPosition(m_ChildAnchorLocal.m_vPosition);
  const plVec3 vJointPos = pJointObject->GetGlobalPosition();
  const float fDistance = (vAnchorPos - vJointPos).GetLength();

  if (fDistance < m_fBreakDistance)
  {
    m_LastValidTime = GetWorld()->GetClock().GetAccumulatedTime();
  }
  else if (fDistance > m_fMaxGrabPointDistance * 1.1f)
  {
    BreakObjectGrab();
    return;
  }
  else
  {
    // TODO: make this configurable?
    if (GetWorld()->GetClock().GetAccumulatedTime() - m_LastValidTime > plTime::MakeFromSeconds(1.0))
    {
      BreakObjectGrab();
      return;
    }
  }
}

bool plJoltGrabObjectComponent::IsCharacterStandingOnObject(plGameObjectHandle hActorToGrab) const
{
  const plJoltCharacterControllerComponent* pController;
  if (GetWorld()->TryGetComponent(m_hCharacterControllerComponent, pController))
  {
    // TODO
    // if (pController->GetStandingOnActor() == hActorToGrab)
    //{
    //  return true;
    //}
  }

  return false;
}

void plJoltGrabObjectComponent::OnMsgReleaseObjectGrab(plMsgReleaseObjectGrab& msg)
{
  if (!msg.m_hGrabbedObjectToRelease.IsInvalidated() && !m_hGrabbedActor.IsInvalidated())
  {
    plComponent* pComponent;
    if (GetOwner()->GetWorld()->TryGetComponent(m_hGrabbedActor, pComponent))
    {
      if (pComponent->GetOwner()->GetHandle() == msg.m_hGrabbedObjectToRelease)
      {
        DropGrabbedObject();
      }
    }
  }
}

void plJoltGrabObjectComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  plGameObject* pObj = GetOwner();

  while (pObj)
  {
    plJoltCharacterControllerComponent* pController;
    if (pObj->TryGetComponentOfBaseType(pController))
    {
      m_hCharacterControllerComponent = pController->GetHandle();
    }

    pObj = pObj->GetParent();
  }
}

void plJoltGrabObjectComponent::OnDeactivated()
{
  ReleaseGrabbedObject();

  SUPER::OnDeactivated();
}

void plJoltGrabObjectComponent::Update()
{
  if (m_pConstraint == nullptr)
    return;

  plJoltDynamicActorComponent* pGrabbedActor;
  if (!GetWorld()->TryGetComponent(m_hGrabbedActor, pGrabbedActor))
  {
    BreakObjectGrab();
    return;
  }

  DetectDistanceViolation(pGrabbedActor);

  if (IsCharacterStandingOnObject(pGrabbedActor->GetOwner()->GetHandle()))
  {
    // disallow grabbing something again until that time
    // to prevent grabbing an object in air that we just jumped off of
    m_LastValidTime = GetWorld()->GetClock().GetAccumulatedTime() + plTime::MakeFromMilliseconds(400);
    BreakObjectGrab();
  }
}


PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltGrabObjectComponent);
