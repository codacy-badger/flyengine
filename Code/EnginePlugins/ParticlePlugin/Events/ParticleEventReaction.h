#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ParticlePlugin/Module/ParticleModule.h>
#include <ParticlePlugin/ParticlePluginDLL.h>

class plParticleEffectInstance;
class plParticleEventReaction;

/// \brief Base class for all particle event reactions
class PL_PARTICLEPLUGIN_DLL plParticleEventReactionFactory : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEventReactionFactory, plReflectedClass);

public:
  virtual const plRTTI* GetEventReactionType() const = 0;
  virtual void CopyReactionProperties(plParticleEventReaction* pObject, bool bFirstTime) const = 0;

  plParticleEventReaction* CreateEventReaction(plParticleEffectInstance* pOwner) const;

  virtual void Save(plStreamWriter& inout_stream) const;
  virtual void Load(plStreamReader& inout_stream);

  plString m_sEventType;
  plUInt8 m_uiProbability = 100;
};

class PL_PARTICLEPLUGIN_DLL plParticleEventReaction : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plParticleEventReaction, plReflectedClass);

  friend class plParticleEventReactionFactory;
  friend class plParticleEffectInstance;

protected:
  plParticleEventReaction();
  ~plParticleEventReaction();

  void Reset(plParticleEffectInstance* pOwner);

  virtual void ProcessEvent(const plParticleEvent& e) = 0;

  plTempHashedString m_sEventName;
  plUInt8 m_uiProbability;
  plParticleEffectInstance* m_pOwnerEffect = nullptr;
};
