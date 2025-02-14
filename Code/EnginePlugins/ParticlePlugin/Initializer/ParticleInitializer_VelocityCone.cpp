#include <ParticlePlugin/ParticlePluginPCH.h>

#include <Foundation/DataProcessing/Stream/ProcessingStreamGroup.h>
#include <Foundation/Math/Random.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>
#include <ParticlePlugin/Finalizer/ParticleFinalizer_ApplyVelocity.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_VelocityCone.h>
#include <ParticlePlugin/System/ParticleSystemInstance.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializerFactory_VelocityCone, 2, plRTTIDefaultAllocator<plParticleInitializerFactory_VelocityCone>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Angle", m_Angle)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(30)), new plClampValueAttribute(plAngle::MakeFromDegree(1), plAngle::MakeFromDegree(89))),
    PL_MEMBER_PROPERTY("Speed", m_Speed),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plConeVisualizerAttribute(plBasisAxis::PositiveZ, "Angle", 1.0f, nullptr, plColor::CornflowerBlue)
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleInitializer_VelocityCone, 1, plRTTIDefaultAllocator<plParticleInitializer_VelocityCone>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleInitializerFactory_VelocityCone::plParticleInitializerFactory_VelocityCone()
{
  m_Angle = plAngle::MakeFromDegree(45);
}

const plRTTI* plParticleInitializerFactory_VelocityCone::GetInitializerType() const
{
  return plGetStaticRTTI<plParticleInitializer_VelocityCone>();
}

void plParticleInitializerFactory_VelocityCone::CopyInitializerProperties(plParticleInitializer* pInitializer0, bool bFirstTime) const
{
  plParticleInitializer_VelocityCone* pInitializer = static_cast<plParticleInitializer_VelocityCone*>(pInitializer0);

  pInitializer->m_Angle = plMath::Clamp(m_Angle, plAngle::MakeFromDegree(1), plAngle::MakeFromDegree(89));
  pInitializer->m_Speed = m_Speed;
}

void plParticleInitializerFactory_VelocityCone::Save(plStreamWriter& inout_stream) const
{
  const plUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_Angle;
  inout_stream << m_Speed.m_Value;
  inout_stream << m_Speed.m_fVariance;
}

void plParticleInitializerFactory_VelocityCone::Load(plStreamReader& inout_stream)
{
  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  inout_stream >> m_Angle;
  inout_stream >> m_Speed.m_Value;
  inout_stream >> m_Speed.m_fVariance;
}

void plParticleInitializerFactory_VelocityCone::QueryFinalizerDependencies(plSet<const plRTTI*>& inout_finalizerDeps) const
{
  inout_finalizerDeps.Insert(plGetStaticRTTI<plParticleFinalizerFactory_ApplyVelocity>());
}

//////////////////////////////////////////////////////////////////////////

void plParticleInitializer_VelocityCone::CreateRequiredStreams()
{
  CreateStream("Velocity", plProcessingStream::DataType::Float3, &m_pStreamVelocity, true);
}

void plParticleInitializer_VelocityCone::InitializeElements(plUInt64 uiStartIndex, plUInt64 uiNumElements)
{
  PL_PROFILE_SCOPE("PFX: Velocity Cone");

  const plVec3 startVel = GetOwnerSystem()->GetParticleStartVelocity();

  plVec3* pVelocity = m_pStreamVelocity->GetWritableData<plVec3>();

  plRandom& rng = GetRNG();

  // const float dist = 1.0f / plMath::Tan(m_Angle);

  for (plUInt64 i = uiStartIndex; i < uiStartIndex + uiNumElements; ++i)
  {
    const plVec3 dir = plVec3::MakeRandomDeviationZ(rng, m_Angle);
    // dir.z = 0;
    // float len = 0.0f;

    // do
    //{
    //  // random point in a rectangle
    //  dir.x = (float)rng.DoubleMinMax(-1.0, 1.0);
    //  dir.y = (float)rng.DoubleMinMax(-1.0, 1.0);

    //  // discard points outside the circle
    //  len = dir.GetLengthSquared();
    //} while (len > 1.0f);

    // dir.z = dist;
    // dir.Normalize();

    const float fSpeed = (float)rng.DoubleVariance(m_Speed.m_Value, m_Speed.m_fVariance);

    pVelocity[i] = startVel + GetOwnerSystem()->GetTransform().m_qRotation * dir * fSpeed;
  }
}

//////////////////////////////////////////////////////////////////////////

class plParticleInitializerFactory_VelocityCone_1_2 : public plGraphPatch
{
public:
  plParticleInitializerFactory_VelocityCone_1_2()
    : plGraphPatch("plParticleInitializerFactory_VelocityCone", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->InlineProperty("Speed").IgnoreResult();
  }
};

plParticleInitializerFactory_VelocityCone_1_2 g_plParticleInitializerFactory_VelocityCone_1_2;

PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_Initializer_ParticleInitializer_VelocityCone);
