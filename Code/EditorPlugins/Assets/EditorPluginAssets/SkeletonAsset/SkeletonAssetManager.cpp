#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetManager.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetWindow.moc.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSkeletonAssetDocumentManager, 1, plRTTIDefaultAllocator<plSkeletonAssetDocumentManager>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plSkeletonAssetDocumentManager::plSkeletonAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plSkeletonAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Skeleton";
  m_DocTypeDesc.m_sFileExtension = "plSkeletonAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Skeleton.svg";
  m_DocTypeDesc.m_sAssetCategory = "Animation";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plSkeletonAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Mesh_Skeleton");

  m_DocTypeDesc.m_sResourceFileExtension = "plSkeleton";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::SupportsThumbnail | plAssetDocumentFlags::AutoTransformOnSave;
}

plSkeletonAssetDocumentManager::~plSkeletonAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plSkeletonAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plSkeletonAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plSkeletonAssetDocument>())
      {
        new plQtSkeletonAssetDocumentWindow(static_cast<plSkeletonAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plSkeletonAssetDocumentManager::InternalCreateDocument(
  plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plSkeletonAssetDocument(sPath);
}

void plSkeletonAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
