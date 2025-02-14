#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorPluginAssets/DecalAsset/DecalAsset.h>
#include <EditorPluginAssets/DecalAsset/DecalAssetManager.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plDecalMode, 1)
  PL_ENUM_CONSTANT(plDecalMode::BaseColor),
  PL_ENUM_CONSTANT(plDecalMode::BaseColorNormal),
  PL_ENUM_CONSTANT(plDecalMode::BaseColorORM),
  PL_ENUM_CONSTANT(plDecalMode::BaseColorNormalORM),
  PL_ENUM_CONSTANT(plDecalMode::BaseColorEmissive)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDecalAssetProperties, 3, plRTTIDefaultAllocator<plDecalAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("Mode", plDecalMode, m_Mode),
    PL_MEMBER_PROPERTY("BlendModeColorize", m_bBlendModeColorize),
    PL_MEMBER_PROPERTY("AlphaMask", m_sAlphaMask)->AddAttributes(new plFileBrowserAttribute("Select Alpha Mask", plFileBrowserAttribute::ImagesLdrOnly)),
    PL_MEMBER_PROPERTY("BaseColor", m_sBaseColor)->AddAttributes(new plFileBrowserAttribute("Select Base Color Map", plFileBrowserAttribute::ImagesLdrOnly)),
    PL_MEMBER_PROPERTY("Normal", m_sNormal)->AddAttributes(new plFileBrowserAttribute("Select Normal Map", plFileBrowserAttribute::ImagesLdrOnly), new plDefaultValueAttribute(plStringView("Textures/NeutralNormal.tga"))), // wrap in plStringView to prevent a memory leak report
    PL_MEMBER_PROPERTY("ORM", m_sORM)->AddAttributes(new plFileBrowserAttribute("Select ORM Map", plFileBrowserAttribute::ImagesLdrOnly)),
    PL_MEMBER_PROPERTY("Emissive", m_sEmissive)->AddAttributes(new plFileBrowserAttribute("Select Emissive Map", plFileBrowserAttribute::ImagesLdrOnly)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDecalAssetProperties::plDecalAssetProperties() = default;

void plDecalAssetProperties::PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plDecalAssetProperties>())
  {
    plInt64 mode = e.m_pObject->GetTypeAccessor().GetValue("Mode").ConvertTo<plInt64>();

    auto& props = *e.m_pPropertyStates;

    props["Normal"].m_Visibility = plPropertyUiState::Invisible;
    props["ORM"].m_Visibility = plPropertyUiState::Invisible;
    props["Emissive"].m_Visibility = plPropertyUiState::Invisible;

    if (mode == plDecalMode::BaseColorNormal)
    {
      props["Normal"].m_Visibility = plPropertyUiState::Default;
    }
    else if (mode == plDecalMode::BaseColorORM)
    {
      props["ORM"].m_Visibility = plPropertyUiState::Default;
    }
    else if (mode == plDecalMode::BaseColorNormalORM)
    {
      props["Normal"].m_Visibility = plPropertyUiState::Default;
      props["ORM"].m_Visibility = plPropertyUiState::Default;
    }
    else if (mode == plDecalMode::BaseColorEmissive)
    {
      props["Emissive"].m_Visibility = plPropertyUiState::Default;
    }
  }
}

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDecalAssetDocument, 5, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plDecalAssetDocument::plDecalAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plDecalAssetProperties>(sDocumentPath, plAssetDocEngineConnection::Simple, true)
{
}

plTransformStatus plDecalAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  return static_cast<plDecalAssetDocumentManager*>(GetAssetDocumentManager())->GenerateDecalTexture(pAssetProfile);
}

plTransformStatus plDecalAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& Unused)
{
  const plDecalAssetProperties* pProp = GetProperties();

  QStringList arguments;
  plStringBuilder temp;

  const plStringBuilder sThumbnail = GetThumbnailFilePath();

  arguments << "-usage";
  arguments << "Color";

  {
    // Thumbnail
    const plStringBuilder sDir = sThumbnail.GetFileDirectory();
    PL_SUCCEED_OR_RETURN(plOSFile::CreateDirectoryStructure(sDir));

    arguments << "-thumbnailOut";
    arguments << QString::fromUtf8(sThumbnail.GetData());

    arguments << "-thumbnailRes";
    arguments << "256";
  }

  {
    plQtEditorApp* pEditorApp = plQtEditorApp::GetSingleton();

    temp.SetFormat("-in0");

    plStringBuilder sAbsPath = pProp->m_sBaseColor;
    if (!pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath))
    {
      return plStatus(plFmt("Failed to make path absolute: '{}'", sAbsPath));
    }

    arguments << temp.GetData();
    arguments << QString(sAbsPath.GetData());

    if (!pProp->m_sAlphaMask.IsEmpty())
    {
      plStringBuilder sAbsPath2 = pProp->m_sAlphaMask;
      if (!pEditorApp->MakeDataDirectoryRelativePathAbsolute(sAbsPath2))
      {
        return plStatus(plFmt("Failed to make path absolute: '{}'", sAbsPath2));
      }

      arguments << "-in1";
      arguments << QString(sAbsPath2.GetData());

      arguments << "-rgb";
      arguments << "in0.rgb";

      arguments << "-a";
      arguments << "in1.r";
    }
    else
    {
      arguments << "-rgba";
      arguments << "in0.rgba";
    }
  }

  PL_SUCCEED_OR_RETURN(plQtEditorApp::GetSingleton()->ExecuteTool("TexConv", arguments, 180, plLog::GetThreadLocalLogSystem()));

  {
    plUInt64 uiThumbnailHash = plAssetCurator::GetSingleton()->GetAssetReferenceHash(GetGuid());
    PL_ASSERT_DEV(uiThumbnailHash != 0, "Thumbnail hash should never be zero when reaching this point!");

    ThumbnailInfo thumbnailInfo;
    thumbnailInfo.SetFileHashAndVersion(uiThumbnailHash, GetAssetTypeVersion());
    AppendThumbnailInfo(sThumbnail, thumbnailInfo);
    InvalidateAssetThumbnail();
  }

  return plStatus(PL_SUCCESS);
}


//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDecalAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plDecalAssetDocumentGenerator>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plDecalAssetDocumentGenerator::plDecalAssetDocumentGenerator()
{
  AddSupportedFileType("tga");
  AddSupportedFileType("dds");
  AddSupportedFileType("jpg");
  AddSupportedFileType("jpeg");
  AddSupportedFileType("png");
}

plDecalAssetDocumentGenerator::~plDecalAssetDocumentGenerator() = default;

void plDecalAssetDocumentGenerator::GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const
{
  const plStringBuilder baseFilename = sAbsInputFile.GetFileName();

  const bool isDecal = (baseFilename.FindSubString_NoCase("decal") != nullptr);

  {
    plAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = isDecal ? plAssetDocGeneratorPriority::HighPriority : plAssetDocGeneratorPriority::LowPriority;
    info.m_sName = "DecalImport.All";
    info.m_sIcon = ":/AssetIcons/Decal.svg";
  }
}

plStatus plDecalAssetDocumentGenerator::Generate(plStringView sInputFileAbs, plStringView sMode, plDynamicArray<plDocument*>& out_generatedDocuments)
{
  plStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  plOSFile::FindFreeFilename(sOutFile);

  auto pApp = plQtEditorApp::GetSingleton();

  plStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  plDocument* pDoc = pApp->CreateDocument(sOutFile, plDocumentFlags::None);
  if (pDoc == nullptr)
    return plStatus("Could not create target document");

  out_generatedDocuments.PushBack(pDoc);

  plDecalAssetDocument* pAssetDoc = plDynamicCast<plDecalAssetDocument*>(pDoc);

  auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
  accessor.SetValue("BaseColor", sInputFileRel.GetView());

  return plStatus(PL_SUCCESS);
}
