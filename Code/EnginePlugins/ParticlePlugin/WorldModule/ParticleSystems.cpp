#include <ParticlePlugin/ParticlePluginPCH.h>

#include <ParticlePlugin/WorldModule/ParticleWorldModule.h>

plParticleSystemInstance* plParticleWorldModule::CreateSystemInstance(
  plUInt32 uiMaxParticles, plWorld* pWorld, plParticleEffectInstance* pOwnerEffect, float fSpawnMultiplier)
{
  PL_LOCK(m_Mutex);

  plParticleSystemInstance* pResult = nullptr;

  if (!m_ParticleSystemFreeList.IsEmpty())
  {
    pResult = m_ParticleSystemFreeList.PeekBack();
    m_ParticleSystemFreeList.PopBack();
  }

  if (pResult == nullptr)
  {
    pResult = &m_ParticleSystems.ExpandAndGetRef();
  }

  pResult->Construct(uiMaxParticles, pWorld, pOwnerEffect, fSpawnMultiplier);

  return pResult;
}

void plParticleWorldModule::DestroySystemInstance(plParticleSystemInstance* pInstance)
{
  PL_LOCK(m_Mutex);

  PL_ASSERT_DEBUG(pInstance != nullptr, "Invalid particle system");
  pInstance->Destruct();
  m_ParticleSystemFreeList.PushBack(pInstance);
}


PL_STATICLINK_FILE(ParticlePlugin, ParticlePlugin_WorldModule_ParticleSystems);
