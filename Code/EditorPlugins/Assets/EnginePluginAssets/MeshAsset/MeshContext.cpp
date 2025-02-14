#include <EnginePluginAssets/EnginePluginAssetsPCH.h>

#include <EnginePluginAssets/MeshAsset/MeshContext.h>
#include <EnginePluginAssets/MeshAsset/MeshView.h>

#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plMeshContext, 1, plRTTIDefaultAllocator<plMeshContext>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_CONSTANT_PROPERTY("DocumentType", (const char*) "Mesh"),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plMeshContext::plMeshContext()
  : plEngineProcessDocumentContext(plEngineProcessDocumentContextFlags::CreateWorld)
{
  m_pMeshObject = nullptr;
}

void plMeshContext::HandleMessage(const plEditorEngineDocumentMsg* pDocMsg)
{
  if (auto* pMsg = plDynamicCast<const plEditorEngineSetMaterialsMsg*>(pDocMsg))
  {
    plMeshComponent* pMesh;
    if (m_pMeshObject && m_pMeshObject->TryGetComponentOfBaseType(pMesh))
    {
      for (plUInt32 i = 0; i < pMsg->m_Materials.GetCount(); ++i)
      {
        plMaterialResourceHandle hMat;

        if (!pMsg->m_Materials[i].IsEmpty())
        {
          hMat = plResourceManager::LoadResource<plMaterialResource>(pMsg->m_Materials[i]);
        }

        pMesh->SetMaterial(i, hMat);
      }
    }

    return;
  }

  if (auto* pMsg = plDynamicCast<const plQuerySelectionBBoxMsgToEngine*>(pDocMsg))
  {
    QuerySelectionBBox(pMsg);
    return;
  }

  if (auto pMsg = plDynamicCast<const plSimpleDocumentConfigMsgToEngine*>(pDocMsg))
  {
    if (pMsg->m_sWhatToDo == "CommonAssetUiState")
    {
      if (pMsg->m_sPayload == "Grid")
      {
        m_bDisplayGrid = pMsg->m_fPayload > 0;
        return;
      }
    }
  }

  plEngineProcessDocumentContext::HandleMessage(pDocMsg);
}

void plMeshContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  PL_LOCK(pWorld->GetWriteMarker());

  plGameObjectDesc obj;
  plMeshComponent* pMesh;

  // Preview Mesh
  {
    obj.m_sName.Assign("MeshPreview");
    pWorld->CreateObject(obj, m_pMeshObject);

    const plTag& tagCastShadows = plTagRegistry::GetGlobalRegistry().RegisterTag("CastShadow");
    m_pMeshObject->SetTag(tagCastShadows);

    plMeshComponent::CreateComponent(m_pMeshObject, pMesh);
    plStringBuilder sMeshGuid;
    plConversionUtils::ToString(GetDocumentGuid(), sMeshGuid);
    m_hMesh = plResourceManager::LoadResource<plMeshResource>(sMeshGuid);
    pMesh->SetMesh(m_hMesh);

    {
      plResourceLock<plMeshResource> pMeshRes(m_hMesh, plResourceAcquireMode::PointerOnly);
      pMeshRes->m_ResourceEvents.AddEventHandler(plMakeDelegate(&plMeshContext::OnResourceEvent, this), m_MeshResourceEventSubscriber);
    }
  }
}

plEngineProcessViewContext* plMeshContext::CreateViewContext()
{
  return PL_DEFAULT_NEW(plMeshViewContext, this);
}

void plMeshContext::DestroyViewContext(plEngineProcessViewContext* pContext)
{
  PL_DEFAULT_DELETE(pContext);
}

bool plMeshContext::UpdateThumbnailViewContext(plEngineProcessViewContext* pThumbnailViewContext)
{
  if (m_bBoundsDirty)
  {
    PL_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
    m_bBoundsDirty = false;
  }
  plBoundingBoxSphere bounds = GetWorldBounds(m_pWorld);

  plMeshViewContext* pMeshViewContext = static_cast<plMeshViewContext*>(pThumbnailViewContext);
  return pMeshViewContext->UpdateThumbnailCamera(bounds);
}


void plMeshContext::QuerySelectionBBox(const plEditorEngineDocumentMsg* pMsg)
{
  if (m_pMeshObject == nullptr)
    return;

  plBoundingBoxSphere bounds = plBoundingBoxSphere::MakeInvalid();

  {
    PL_LOCK(m_pWorld->GetWriteMarker());

    m_pMeshObject->UpdateLocalBounds();
    m_pMeshObject->UpdateGlobalTransformAndBounds();
    m_bBoundsDirty = false;
    const auto& b = m_pMeshObject->GetGlobalBounds();

    if (b.IsValid())
      bounds.ExpandToInclude(b);
  }

  const plQuerySelectionBBoxMsgToEngine* msg = static_cast<const plQuerySelectionBBoxMsgToEngine*>(pMsg);

  plQuerySelectionBBoxResultMsgToEditor res;
  res.m_uiViewID = msg->m_uiViewID;
  res.m_iPurpose = msg->m_iPurpose;
  res.m_vCenter = bounds.m_vCenter;
  res.m_vHalfExtents = bounds.m_vBoxHalfExtends;
  res.m_DocumentGuid = pMsg->m_DocumentGuid;

  SendProcessMessage(&res);
}

void plMeshContext::OnResourceEvent(const plResourceEvent& e)
{
  if (e.m_Type == plResourceEvent::Type::ResourceContentUpdated)
  {
    m_bBoundsDirty = true;
  }
}
