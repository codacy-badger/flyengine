#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/RenderTargetActivatorComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plRenderTargetActivatorComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("RenderTarget", GetRenderTargetFile, SetRenderTargetFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_Target", plDependencyFlags::Package)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_COMPONENT_TYPE;
// clang-format on

plRenderTargetActivatorComponent::plRenderTargetActivatorComponent() = default;
plRenderTargetActivatorComponent::~plRenderTargetActivatorComponent() = default;

void plRenderTargetActivatorComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_hRenderTarget;
}

void plRenderTargetActivatorComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();

  s >> m_hRenderTarget;
}

plResult plRenderTargetActivatorComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  if (m_hRenderTarget.IsValid())
  {
    ref_bounds = plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), 0.1f);
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

void plRenderTargetActivatorComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // only add render target views from main views
  // otherwise every shadow casting light source would activate a render target
  if (msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::MainView && msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::EditorView)
    return;

  if (!m_hRenderTarget.IsValid())
    return;

  plResourceLock<plRenderToTexture2DResource> pRenderTarget(m_hRenderTarget, plResourceAcquireMode::BlockTillLoaded);

  for (auto hView : pRenderTarget->GetAllRenderViews())
  {
    plRenderWorld::AddViewToRender(hView);
  }
}

void plRenderTargetActivatorComponent::SetRenderTarget(const plRenderToTexture2DResourceHandle& hResource)
{
  m_hRenderTarget = hResource;

  TriggerLocalBoundsUpdate();
}

void plRenderTargetActivatorComponent::SetRenderTargetFile(const char* szFile)
{
  plRenderToTexture2DResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plRenderToTexture2DResource>(szFile);
  }

  SetRenderTarget(hResource);
}

const char* plRenderTargetActivatorComponent::GetRenderTargetFile() const
{
  if (!m_hRenderTarget.IsValid())
    return "";

  return m_hRenderTarget.GetResourceID();
}



PL_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_RenderTargetActivatorComponent);
