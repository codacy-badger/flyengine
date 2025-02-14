#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetManager.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAssetWindow.moc.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEffectAssetDocumentManager, 1, plRTTIDefaultAllocator<plParticleEffectAssetDocumentManager>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plParticleEffectAssetDocumentManager::plParticleEffectAssetDocumentManager()
{
  plDocumentManager::s_Events.AddEventHandler(plMakeDelegate(&plParticleEffectAssetDocumentManager::OnDocumentManagerEvent, this));

  m_DocTypeDesc.m_sDocumentTypeName = "Particle Effect";
  m_DocTypeDesc.m_sFileExtension = "plParticleEffectAsset";
  m_DocTypeDesc.m_sIcon = ":/AssetIcons/Particle_Effect.svg";
  m_DocTypeDesc.m_sAssetCategory = "Effects";
  m_DocTypeDesc.m_pDocumentType = plGetStaticRTTI<plParticleEffectAssetDocument>();
  m_DocTypeDesc.m_pManager = this;
  m_DocTypeDesc.m_CompatibleTypes.PushBack("CompatibleAsset_Particle_Effect");

  m_DocTypeDesc.m_sResourceFileExtension = "plParticleEffect";
  m_DocTypeDesc.m_AssetDocumentFlags = plAssetDocumentFlags::AutoTransformOnSave | plAssetDocumentFlags::SupportsThumbnail;
}

plParticleEffectAssetDocumentManager::~plParticleEffectAssetDocumentManager()
{
  plDocumentManager::s_Events.RemoveEventHandler(plMakeDelegate(&plParticleEffectAssetDocumentManager::OnDocumentManagerEvent, this));
}

void plParticleEffectAssetDocumentManager::OnDocumentManagerEvent(const plDocumentManager::Event& e)
{
  switch (e.m_Type)
  {
    case plDocumentManager::Event::Type::DocumentWindowRequested:
    {
      if (e.m_pDocument->GetDynamicRTTI() == plGetStaticRTTI<plParticleEffectAssetDocument>())
      {
        new plQtParticleEffectAssetDocumentWindow(static_cast<plParticleEffectAssetDocument*>(e.m_pDocument)); // NOLINT: Not a memory leak
      }
    }
    break;

    default:
      break;
  }
}

void plParticleEffectAssetDocumentManager::InternalCreateDocument(
  plStringView sDocumentTypeName, plStringView sPath, bool bCreateNewDocument, plDocument*& out_pDocument, const plDocumentObject* pOpenContext)
{
  out_pDocument = new plParticleEffectAssetDocument(sPath);
}

void plParticleEffectAssetDocumentManager::InternalGetSupportedDocumentTypes(
  plDynamicArray<const plDocumentTypeDescriptor*>& inout_DocumentTypes) const
{
  inout_DocumentTypes.PushBack(&m_DocTypeDesc);
}
