#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <GameEngine/StateMachine/StateMachineResource.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineResource, 1, plRTTIDefaultAllocator<plStateMachineResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plStateMachineResource);
// clang-format on

plStateMachineResource::plStateMachineResource()
  : plResource(DoUpdate::OnAnyThread, 1)
{
}

plStateMachineResource::~plStateMachineResource() = default;

plUniquePtr<plStateMachineInstance> plStateMachineResource::CreateInstance(plReflectedClass& ref_owner)
{
  if (m_pDescription != nullptr)
  {
    return PL_DEFAULT_NEW(plStateMachineInstance, ref_owner, m_pDescription);
  }

  return nullptr;
}

plResourceLoadDesc plStateMachineResource::UnloadData(Unload WhatToUnload)
{
  m_pDescription = nullptr;

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;
  res.m_State = plResourceState::Unloaded;

  return res;
}

plResourceLoadDesc plStateMachineResource::UpdateContent(plStreamReader* Stream)
{
  PL_LOG_BLOCK("plStateMachineResource::UpdateContent", GetResourceIdOrDescription());

  plResourceLoadDesc res;
  res.m_uiQualityLevelsDiscardable = 0;
  res.m_uiQualityLevelsLoadable = 0;

  if (Stream == nullptr)
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  // skip the absolute file path data that the standard file reader writes into the stream
  {
    plStringBuilder sAbsFilePath;
    (*Stream) >> sAbsFilePath;
  }

  plAssetFileHeader AssetHash;
  AssetHash.Read(*Stream).IgnoreResult();

  plUniquePtr<plStateMachineDescription> pDescription = PL_DEFAULT_NEW(plStateMachineDescription);
  if (pDescription->Deserialize(*Stream).Failed())
  {
    res.m_State = plResourceState::LoadedResourceMissing;
    return res;
  }

  m_pDescription = std::move(pDescription);

  res.m_State = plResourceState::Loaded;
  return res;
}

void plStateMachineResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = 0;
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}


PL_STATICLINK_FILE(GameEngine, GameEngine_StateMachine_Implementation_StateMachineResource);
