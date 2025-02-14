#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetManager.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <ToolsFoundation/Assets/AssetFileExtensionWhitelist.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureAssetProfileConfig, 1, plRTTIDefaultAllocator<plTextureAssetProfileConfig>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("MaxResolution", m_uiMaxResolution)->AddAttributes(new plDefaultValueAttribute(16 * 1024)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureAssetDocumentManager, 1, plRTTIDefaultAllocator<plTextureAssetDocumentManager>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plTextureAssetDocumentManager::plTextureAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plTextureAssetDocumentManager::OnDocumentManagerEvent, this));

  // additional whitelist for non-asset files where an asset may be selected
  plAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_2D", "dds");
  plAssetFileExtensionWhitelist::AddAssetFileExtension("CompatibleAsset_Texture_2D", "color");

  // texture asset source files
  plAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "dds");
  plAssetFileExtensionWhitelist::AddAssetFileExtension("Image2D", "tga");

  m_DocTypeDesc.m_sDocumentTypeName = "Texture 2D";
  m_DocTypeDesc.m_sFileExtension = "plTextureAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Texture_2D.svg";
  m_DocTypeDesc.m_sAssetCategory = "Rendering";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plTextureAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_sResourceFileExtension = "plTexture2D";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoThumbnailOnTransform;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_2D");

  m_DocTypeDesc2.m_sDocumentTypeName = "Render Target";
  m_DocTypeDesc2.m_sFileExtension = "plRenderTargetAsset";
  m_DocTypeDesc2.m_sIcon = ":/AssetIcons/Render_Target.svg";
  m_DocTypeDesc2.m_sAssetCategory = "Rendering";
  m_DocTypeDesc2.m_pDocumentType = plGetStaticRTTI<plTextureAssetDocument>();
  m_DocTypeDesc2.m_pManager = this;
  m_DocTypeDesc2.m_sResourceFileExtension = "plRenderTarget";
  m_DocTypeDesc2.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave;
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_2D"); // render targets can also be used as 2D textures
  m_DocTypeDesc2.m_CompatibleTypes.PushBack("CompatibleAsset_Texture_Target");

  plQtImageCache::GetSingleton()->RegisterTypeImage("Render Target", QPixmap(":/AssetIcons/Render_Target.svg"));
}

plTextureAssetDocumentManager::~plTextureAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plTextureAssetDocumentManager::OnDocumentManagerEvent, this));
}

plUInt64 plTextureAssetDocumentManager::ComputeAssetProfileHashImpl(const plPlatformProfile* pAssetProfile) const
{
  return pAssetProfile->GetTypeConfig<plTextureAssetProfileConfig>()->m_uiMaxResolution;
}

void plTextureAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plTextureAssetDocument>())
      {
        new plQtTextureAssetDocumentWindow(static_cast<plTextureAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plTextureAssetDocumentManager::InternalCreateDocument(plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  plTextureAssetDocument* pDoc = new plTextureAssetDocument(sPath);
  out_pDocument = pDoc;

  if (sDocumentTypeName.IsEqual("Render Target"))
  {
    pDoc->m_bIsRenderTarget = true;
  }
}

void plTextureAssetDocumentManager::InternalGetSupportedDocumentTypes(plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
  inout_DocumentTypes.PushBack(&m_DocTypeDesc2);
}

plString plTextureAssetDocumentManager::GetRelativeOutputFileName(const plAssetDocumentTypeDescriptor* pTypeDescriptor, plStringView sDataDirectory, plStringView sDocumentPath, plStringView sOutputTag, const plPlatformProfile* pAssetProfile) const
{
  if (sOutputTag.IsEqual("LOWRES"))
  {
    plStringBuilder sRelativePath(sDocumentPath);
    sRelativePath.MakeRelativeTo(sDataDirectory).IgnoreResult();
    sRelativePath.RemoveFileExtension();
    sRelativePath.Append("-lowres");
    plAssetDocumentManager::GenerateOutputFilename(sRelativePath, pAssetProfile, "plTexture2D", true);
    return sRelativePath;
  }

  return SUPER::GetRelativeOutputFileName(pTypeDescriptor, sDataDirectory, sDocumentPath, sOutputTag, pAssetProfile);
}
