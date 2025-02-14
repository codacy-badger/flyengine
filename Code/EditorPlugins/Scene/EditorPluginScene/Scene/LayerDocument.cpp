#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorPluginScene/Scene/LayerDocument.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <SharedPluginScene/Common/Messages.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLayerDocument, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plLayerDocument::plLayerDocument(plStringView sDocumentPath, plScene2Document* pParentScene)
  : plSceneDocument(sDocumentPath, plSceneDocument::DocumentType::Layer)
{
  m_pHostDocument = pParentScene;
}

plLayerDocument::~plLayerDocument() = default;

void plLayerDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  SUPER::InitializeAfterLoading(bFirstTimeCreation);
}

void plLayerDocument::InitializeAfterLoadingAndSaving()
{
  SUPER::InitializeAfterLoadingAndSaving();
}

plVariant plLayerDocument::GetCreateEngineMetaData() const
{
  return m_pHostDocument->GetGuid();
}
