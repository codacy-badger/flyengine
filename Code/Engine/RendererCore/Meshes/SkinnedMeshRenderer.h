#pragma once

#include <RendererCore/Meshes/MeshRenderer.h>

/// \brief Implements rendering of skinned meshes
class PL_RENDERERCORE_DLL plSkinnedMeshRenderer : public plMeshRenderer
{
  PL_ADD_DYNAMIC_REFLECTION(plSkinnedMeshRenderer, plMeshRenderer);
  PL_DISALLOW_COPY_AND_ASSIGN(plSkinnedMeshRenderer);

public:
  plSkinnedMeshRenderer();
  ~plSkinnedMeshRenderer();

  // plRenderer implementation
  virtual void GetSupportedRenderDataTypes(plHybridArray<const plRTTI*, 8>& ref_types) const override;

protected:
  virtual void SetAdditionalData(const plRenderViewContext& renderViewContext, const plMeshRenderData* pRenderData) const override;

  static plUInt32 s_uiSkinningBufferUpdates;
};
