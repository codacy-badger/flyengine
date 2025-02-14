#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/TriggerMessage.h>
#include <Core/Prefabs/PrefabReferenceComponent.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/SpawnComponent.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plSpawnComponent, 3, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Prefab", GetPrefabFile, SetPrefabFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Prefab", plDependencyFlags::Package)),
    PL_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new plExposedParametersAttribute("Prefab")),
    PL_ACCESSOR_PROPERTY("AttachAsChild", GetAttachAsChild, SetAttachAsChild),
    PL_ACCESSOR_PROPERTY("SpawnAtStart", GetSpawnAtStart, SetSpawnAtStart),
    PL_ACCESSOR_PROPERTY("SpawnContinuously", GetSpawnContinuously, SetSpawnContinuously),
    PL_MEMBER_PROPERTY("MinDelay", m_MinDelay)->AddAttributes(new plClampValueAttribute(plTime(), plVariant()), new plDefaultValueAttribute(plTime::MakeFromSeconds(1.0))),
    PL_MEMBER_PROPERTY("DelayRange", m_DelayRange)->AddAttributes(new plClampValueAttribute(plTime(), plVariant())),
    PL_MEMBER_PROPERTY("Deviation", m_MaxDeviation)->AddAttributes(new plClampValueAttribute(plAngle(), plAngle::MakeFromDegree(179.0))),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 0.5f, plColorScheme::LightUI(plColorScheme::Lime)),
    new plConeVisualizerAttribute(plBasisAxis::PositiveX, "Deviation", 0.5f, nullptr, plColorScheme::LightUI(plColorScheme::Lime)),
    new plConeAngleManipulatorAttribute("Deviation", 0.5f),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnTriggered),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(CanTriggerManualSpawn),
    PL_SCRIPT_FUNCTION_PROPERTY(TriggerManualSpawn, In, "IgnoreSpawnDelay", In, "LocalOffset"),
    PL_SCRIPT_FUNCTION_PROPERTY(ScheduleSpawn),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSpawnComponent::plSpawnComponent() = default;
plSpawnComponent::~plSpawnComponent() = default;

void plSpawnComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_SpawnFlags.IsAnySet(plSpawnComponentFlags::SpawnAtStart))
  {
    ScheduleSpawn();
  }
}


void plSpawnComponent::OnDeactivated()
{
  m_SpawnFlags.Remove(plSpawnComponentFlags::SpawnInFlight);

  SUPER::OnDeactivated();
}

bool plSpawnComponent::SpawnOnce(const plVec3& vLocalOffset)
{
  if (m_hPrefab.IsValid())
  {
    plTransform tLocalSpawn;
    tLocalSpawn.SetIdentity();
    tLocalSpawn.m_vPosition = vLocalOffset;

    if (m_MaxDeviation.GetRadian() > 0)
    {
      const plVec3 vTiltAxis = plVec3(0, 1, 0);
      const plVec3 vTurnAxis = plVec3(1, 0, 0);

      const plAngle tiltAngle = plAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, (double)m_MaxDeviation.GetRadian()));
      const plAngle turnAngle = plAngle::MakeFromRadian((float)GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, plMath::Pi<double>() * 2.0));

      plQuat qTilt, qTurn, qDeviate;
      qTilt = plQuat::MakeFromAxisAndAngle(vTiltAxis, tiltAngle);
      qTurn = plQuat::MakeFromAxisAndAngle(vTurnAxis, turnAngle);
      qDeviate = qTurn * qTilt;

      tLocalSpawn.m_qRotation = qDeviate;
    }

    DoSpawn(tLocalSpawn);

    return true;
  }

  return false;
}


void plSpawnComponent::DoSpawn(const plTransform& tLocalSpawn)
{
  plResourceLock<plPrefabResource> pResource(m_hPrefab, plResourceAcquireMode::AllowLoadingFallback);

  plPrefabInstantiationOptions options;
  options.m_pOverrideTeamID = &GetOwner()->GetTeamID();

  if (m_SpawnFlags.IsAnySet(plSpawnComponentFlags::AttachAsChild))
  {
    options.m_hParent = GetOwner()->GetHandle();

    pResource->InstantiatePrefab(*GetWorld(), tLocalSpawn, options, &m_Parameters);
  }
  else
  {
    plTransform tGlobalSpawn;
    tGlobalSpawn = plTransform::MakeGlobalTransform(GetOwner()->GetGlobalTransform(), tLocalSpawn);

    pResource->InstantiatePrefab(*GetWorld(), tGlobalSpawn, options, &m_Parameters);
  }
}

void plSpawnComponent::ScheduleSpawn()
{
  if (m_SpawnFlags.IsAnySet(plSpawnComponentFlags::SpawnInFlight))
    return;

  plMsgComponentInternalTrigger msg;
  msg.m_sMessage.Assign("scheduled_spawn");

  m_SpawnFlags.Add(plSpawnComponentFlags::SpawnInFlight);

  plWorld* pWorld = GetWorld();

  const plTime tKill = plTime::MakeFromSeconds(pWorld->GetRandomNumberGenerator().DoubleInRange(m_MinDelay.GetSeconds(), m_DelayRange.GetSeconds()));

  PostMessage(msg, tKill);
}

void plSpawnComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_SpawnFlags.GetValue();
  s << m_hPrefab;

  s << m_MinDelay;
  s << m_DelayRange;
  s << m_MaxDeviation;
  s << m_LastManualSpawn;

  plPrefabReferenceComponent::SerializePrefabParameters(*GetWorld(), inout_stream, m_Parameters);
}

void plSpawnComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  plSpawnComponentFlags::StorageType flags;
  s >> flags;
  m_SpawnFlags.SetValue(flags);

  s >> m_hPrefab;

  s >> m_MinDelay;
  s >> m_DelayRange;
  s >> m_MaxDeviation;
  s >> m_LastManualSpawn;

  if (uiVersion >= 3)
  {
    plPrefabReferenceComponent::DeserializePrefabParameters(m_Parameters, inout_stream);
  }
}

bool plSpawnComponent::CanTriggerManualSpawn() const
{
  const plTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  return tNow - m_LastManualSpawn >= m_MinDelay;
}

bool plSpawnComponent::TriggerManualSpawn(bool bIgnoreSpawnDelay /*= false*/, const plVec3& vLocalOffset /*= plVec3::MakeZero()*/)
{
  const plTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

  if (bIgnoreSpawnDelay == false && tNow - m_LastManualSpawn < m_MinDelay)
    return false;

  m_LastManualSpawn = tNow;
  return SpawnOnce(vLocalOffset);
}

void plSpawnComponent::SetPrefabFile(const char* szFile)
{
  plPrefabResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plPrefabResource>(szFile);
  }

  SetPrefab(hResource);
}

const char* plSpawnComponent::GetPrefabFile() const
{
  if (!m_hPrefab.IsValid())
    return "";

  return m_hPrefab.GetResourceID();
}

bool plSpawnComponent::GetSpawnAtStart() const
{
  return m_SpawnFlags.IsAnySet(plSpawnComponentFlags::SpawnAtStart);
}

void plSpawnComponent::SetSpawnAtStart(bool b)
{
  m_SpawnFlags.AddOrRemove(plSpawnComponentFlags::SpawnAtStart, b);
}

bool plSpawnComponent::GetSpawnContinuously() const
{
  return m_SpawnFlags.IsAnySet(plSpawnComponentFlags::SpawnContinuously);
}

void plSpawnComponent::SetSpawnContinuously(bool b)
{
  m_SpawnFlags.AddOrRemove(plSpawnComponentFlags::SpawnContinuously, b);
}

bool plSpawnComponent::GetAttachAsChild() const
{
  return m_SpawnFlags.IsAnySet(plSpawnComponentFlags::AttachAsChild);
}

void plSpawnComponent::SetAttachAsChild(bool b)
{
  m_SpawnFlags.AddOrRemove(plSpawnComponentFlags::AttachAsChild, b);
}

void plSpawnComponent::SetPrefab(const plPrefabResourceHandle& hPrefab)
{
  m_hPrefab = hPrefab;
}

void plSpawnComponent::OnTriggered(plMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage == plTempHashedString("scheduled_spawn"))
  {
    m_SpawnFlags.Remove(plSpawnComponentFlags::SpawnInFlight);

    SpawnOnce(plVec3::MakeZero());

    // do it all again
    if (m_SpawnFlags.IsAnySet(plSpawnComponentFlags::SpawnContinuously))
    {
      ScheduleSpawn();
    }
  }
  else if (msg.m_sMessage == plTempHashedString("spawn"))
  {
    TriggerManualSpawn();
  }
}

const plRangeView<const char*, plUInt32> plSpawnComponent::GetParameters() const
{
  return plRangeView<const char*, plUInt32>([]() -> plUInt32 { return 0; }, [this]() -> plUInt32 { return m_Parameters.GetCount(); }, [](plUInt32& ref_uiIt) { ++ref_uiIt; }, [this](const plUInt32& uiIt) -> const char* { return m_Parameters.GetKey(uiIt).GetString().GetData(); });
}

void plSpawnComponent::SetParameter(const char* szKey, const plVariant& value)
{
  plHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != plInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;
}

void plSpawnComponent::RemoveParameter(const char* szKey)
{
  m_Parameters.RemoveAndCopy(plTempHashedString(szKey));
}

bool plSpawnComponent::GetParameter(const char* szKey, plVariant& out_value) const
{
  plUInt32 it = m_Parameters.Find(szKey);

  if (it == plInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plSpawnComponentPatch_1_2 : public plGraphPatch
{
public:
  plSpawnComponentPatch_1_2()
    : plGraphPatch("plSpawnComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Attach as Child", "AttachAsChild");
    pNode->RenameProperty("Spawn at Start", "SpawnAtStart");
    pNode->RenameProperty("Spawn Continuously", "SpawnContinuously");
    pNode->RenameProperty("Min Delay", "MinDelay");
    pNode->RenameProperty("Delay Range", "DelayRange");
  }
};

plSpawnComponentPatch_1_2 g_plSpawnComponentPatch_1_2;



PL_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_SpawnComponent);
