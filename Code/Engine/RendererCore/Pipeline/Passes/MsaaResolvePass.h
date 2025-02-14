#pragma once

#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <RendererCore/Shader/ShaderResource.h>

class PL_RENDERERCORE_DLL plMsaaResolvePass : public plRenderPipelinePass
{
  PL_ADD_DYNAMIC_REFLECTION(plMsaaResolvePass, plRenderPipelinePass);

public:
  plMsaaResolvePass();
  ~plMsaaResolvePass();

  virtual bool GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs) override;

  virtual void Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs) override;

protected:
  plRenderPipelineNodeInputPin m_PinInput;
  plRenderPipelineNodeOutputPin m_PinOutput;

  bool m_bIsDepth = false;
  plGALMSAASampleCount::Enum m_MsaaSampleCount = plGALMSAASampleCount::None;
  plShaderResourceHandle m_hDepthResolveShader;
};
