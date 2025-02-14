#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Core/ResourceManager/Resource.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>

using plBlackboardTemplateResourceHandle = plTypedResourceHandle<class plBlackboardTemplateResource>;

struct PL_GAMEENGINE_DLL plBlackboardTemplateResourceDescriptor
{
  plResult Serialize(plStreamWriter& inout_stream) const;
  plResult Deserialize(plStreamReader& inout_stream);

  plDynamicArray<plBlackboardEntry> m_Entries;
};

/// \brief Describes the initial state of a blackboard.
///
/// Used by plBlackboardComponent to initialize its blackboard from.
class PL_GAMEENGINE_DLL plBlackboardTemplateResource : public plResource
{
  PL_ADD_DYNAMIC_REFLECTION(plBlackboardTemplateResource, plResource);
  PL_RESOURCE_DECLARE_COMMON_CODE(plBlackboardTemplateResource);
  PL_RESOURCE_DECLARE_CREATEABLE(plBlackboardTemplateResource, plBlackboardTemplateResourceDescriptor);

public:
  plBlackboardTemplateResource();
  ~plBlackboardTemplateResource();

  const plBlackboardTemplateResourceDescriptor& GetDescriptor() const { return m_Descriptor; }

private:
  virtual plResourceLoadDesc UnloadData(Unload WhatToUnload) override;
  virtual plResourceLoadDesc UpdateContent(plStreamReader* Stream) override;
  virtual void UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage) override;

  plBlackboardTemplateResourceDescriptor m_Descriptor;
};
