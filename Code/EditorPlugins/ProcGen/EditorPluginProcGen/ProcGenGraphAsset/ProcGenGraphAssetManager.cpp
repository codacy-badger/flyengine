#include <EditorPluginProcGen/EditorPluginProcGenPCH.h>

#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAsset.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetManager.h>
#include <EditorPluginProcGen/ProcGenGraphAsset/ProcGenGraphAssetWindow.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plProcGenGraphAssetDocumentManager, 1, plRTTIDefaultAllocator<plProcGenGraphAssetDocumentManager>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plProcGenGraphAssetDocumentManager::plProcGenGraphAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plProcGenGraphAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "ProcGen Graph";
  m_DocTypeDesc.m_sFileExtension = "plProcGenGraphAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/ProcGen_Graph.svg";
  m_DocTypeDesc.m_sAssetCategory = "Construction";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plProcGenGraphAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_ProcGen_Graph");

  m_DocTypeDesc.m_sResourceFileExtension = "plProcGenGraph";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave;

  plQtImageCache::GetSingleton()->RegisterTypeImage("ProcGen Graph", QPixmap(":/AssetIcons/ProcGen_Graph.svg"));
}

plProcGenGraphAssetDocumentManager::~plProcGenGraphAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plProcGenGraphAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plProcGenGraphAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plProcGenGraphAssetDocument>())
      {
        new plProcGenGraphAssetDocumentWindow(static_cast<plProcGenGraphAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plProcGenGraphAssetDocumentManager::InternalCreateDocument(
  plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plProcGenGraphAssetDocument(sPath);
}

void plProcGenGraphAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
