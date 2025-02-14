#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Core/Curves/Curve1DResource.h>
#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>
#include <ParticlePlugin/Emitter/ParticleEmitter_OnEvent.h>
#include <ParticlePlugin/Events/ParticleEvent.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEmitterFactory_OnEvent, 1, plRTTIDefaultAllocator<plParticleEmitterFactory_OnEvent>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("EventName", m_sEventName),
    PL_MEMBER_PROPERTY("MinSpawnCount", m_uiSpawnCountMin)->AddAttributes(new plDefaultValueAttribute(1)),
    PL_MEMBER_PROPERTY("SpawnCountRange", m_uiSpawnCountRange),
    PL_MEMBER_PROPERTY("SpawnCountScaleParam", m_sSpawnCountScaleParameter),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEmitter_OnEvent, 1, plRTTIDefaultAllocator<plParticleEmitter_OnEvent>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleEmitterFactory_OnEvent::plParticleEmitterFactory_OnEvent() = default;
plParticleEmitterFactory_OnEvent::~plParticleEmitterFactory_OnEvent() = default;

const plRTTI* plParticleEmitterFactory_OnEvent::GetEmitterType() const
{
  return plGetStaticRTTI<plParticleEmitter_OnEvent>();
}

void plParticleEmitterFactory_OnEvent::CopyEmitterProperties(plParticleEmitter* pEmitter0, bool bFirstTime) const
{
  plParticleEmitter_OnEvent* pEmitter = static_cast<plParticleEmitter_OnEvent*>(pEmitter0);

  pEmitter->m_sEventName = plTempHashedString(m_sEventName.GetData());

  pEmitter->m_uiSpawnCountMin = (plUInt32)(m_uiSpawnCountMin * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());
  pEmitter->m_uiSpawnCountRange = (plUInt32)(m_uiSpawnCountRange * pEmitter->GetOwnerSystem()->GetSpawnCountMultiplier());

  pEmitter->m_sSpawnCountScaleParameter = plTempHashedString(m_sSpawnCountScaleParameter.GetData());
}

void plParticleEmitterFactory_OnEvent::QueryMaxParticleCount(plUInt32& out_uiMaxParticlesAbs, plUInt32& out_uiMaxParticlesPerSecond) const
{
  out_uiMaxParticlesAbs = 0;
  out_uiMaxParticlesPerSecond = (m_uiSpawnCountMin + m_uiSpawnCountRange) * 16; // some wild guess

  // TODO: consider to scale by m_sSpawnCountScaleParameter
}

enum class EmitterOnEventVersion
{
  Version_0 = 0,
  Version_1,
  Version_2,

  // insert new version numbers above
  Version_Count,
  Version_Current = Version_Count - 1
};


void plParticleEmitterFactory_OnEvent::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = (int)EmitterOnEventVersion::Version_Current;
  inout_stream << uiVersion;

  // Version 1
  inout_stream << m_sEventName;

  // Version 2
  inout_stream << m_uiSpawnCountMin;
  inout_stream << m_uiSpawnCountRange;
  inout_stream << m_sSpawnCountScaleParameter;
}

void plParticleEmitterFactory_OnEvent::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  PL_ASSERT_DEV(uiVersion <= (int)EmitterOnEventVersion::Version_Current, "Invalid version {0}", uiVersion);

  inout_stream >> m_sEventName;

  if (uiVersion >= 2)
  {
    inout_stream >> m_uiSpawnCountMin;
    inout_stream >> m_uiSpawnCountRange;
    inout_stream >> m_sSpawnCountScaleParameter;
  }
}

plParticleEmitterState plParticleEmitter_OnEvent::IsFinished()
{
  return plParticleEmitterState::OnlyReacting;
}

plUInt32 plParticleEmitter_OnEvent::ComputeSpawnCount(const plTime& tDiff)
{
  if (!m_bSpawn)
    return 0;

  m_bSpawn = false;

  float fSpawnFactor = 1.0f;

  const float spawnCountScale = plMath::Max(GetOwnerEffect()->GetFloatParameter(m_sSpawnCountScaleParameter, 1.0f), 0.0f);
  fSpawnFactor *= spawnCountScale;

  plRandom& rng = GetRNG();

  return static_cast<plUInt32>((m_uiSpawnCountMin + GetRNG().UIntInRange(1 + m_uiSpawnCountRange)) * fSpawnFactor);
}

void plParticleEmitter_OnEvent::ProcessEventQueue(plParticleEventQueue queue)
{
  if (m_bSpawn)
    return;

  for (const plParticleEvent& e : queue)
  {
    if (e.m_EventType == m_sEventName) // this is the event type we are waiting for!
    {
      m_bSpawn = true;
      return;
    }
  }
}



PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Emitter_ParticleEmitter_OnEvent);
