#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetBrowserDlg.moc.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAsset.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <ModelImporter2/ModelImporter.h>
#include <RendererCore/AnimationSystem/AnimationClipResource.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plRootMotionSource, 1)
  PL_ENUM_CONSTANTS(plRootMotionSource::None, plRootMotionSource::Constant)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimationClipAssetProperties, 3, plRTTIDefaultAllocator<plAnimationClipAssetProperties>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("File", m_sSourceFile)->AddAttributes(new plFileBrowserAttribute("Select Animation", plFileBrowserAttribute::MeshesWithAnimations)),
    PL_MEMBER_PROPERTY("PreviewMesh", m_sPreviewMesh)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Mesh_Skinned", plDependencyFlags::None)),
    PL_MEMBER_PROPERTY("UseAnimationClip", m_sAnimationClipToExtract),
    PL_ARRAY_MEMBER_PROPERTY("AvailableClips", m_AvailableClips)->AddAttributes(new plReadOnlyAttribute, new plContainerAttribute(false, false, false)),
    PL_MEMBER_PROPERTY("FirstFrame", m_uiFirstFrame),
    PL_MEMBER_PROPERTY("NumFrames", m_uiNumFrames),
    PL_MEMBER_PROPERTY("Additive", m_bAdditive),
    PL_ENUM_MEMBER_PROPERTY("RootMotion", plRootMotionSource, m_RootMotionMode),
    PL_MEMBER_PROPERTY("ConstantRootMotion", m_vConstantRootMotion),
    PL_MEMBER_PROPERTY("EventTrack", m_EventTrack)->AddAttributes(new plHiddenAttribute()),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimationClipAssetDocument, 5, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAnimationClipAssetProperties::plAnimationClipAssetProperties() = default;
plAnimationClipAssetProperties::~plAnimationClipAssetProperties() = default;

void plAnimationClipAssetProperties::PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() != plGetStaticRTTI<plAnimationClipAssetProperties>())
    return;

  auto& props = *e.m_pPropertyStates;

  const plInt64 motionType = e.m_pObject->GetTypeAccessor().GetValue("RootMotion").ConvertTo<plInt64>();

  switch (motionType)
  {
    case plRootMotionSource::Constant:
      props["ConstantRootMotion"].m_Visibility = plPropertyUiState::Default;
      break;

    default:
      props["ConstantRootMotion"].m_Visibility = plPropertyUiState::Invisible;
      break;
  }
}

plAnimationClipAssetDocument::plAnimationClipAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plAnimationClipAssetProperties>(sDocumentPath, plAssetDocEngineConnection::Simple, true)
{
}

void plAnimationClipAssetDocument::SetCommonAssetUiState(plCommonAssetUiState::Enum state, double value)
{
  switch (state)
  {
    case plCommonAssetUiState::SimulationSpeed:
      m_fSimulationSpeed = value;
      break;
    default:
      break;
  }

  // handles standard booleans and broadcasts the event
  return SUPER::SetCommonAssetUiState(state, value);
}

double plAnimationClipAssetDocument::GetCommonAssetUiState(plCommonAssetUiState::Enum state) const
{
  switch (state)
  {
    case plCommonAssetUiState::SimulationSpeed:
      return m_fSimulationSpeed;
    default:
      break;
  }

  return SUPER::GetCommonAssetUiState(state);
}

plTransformStatus plAnimationClipAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plProgressRange range("Transforming Asset", 2, false);

  plAnimationClipAssetProperties* pProp = GetProperties();

  plAnimationClipResourceDescriptor desc;

  range.BeginNextStep("Importing Animations");

  plStringBuilder sAbsFilename = pProp->m_sSourceFile;
  if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(sAbsFilename))
  {
    return plStatus(plFmt("Could not make path absolute: '{0};", sAbsFilename));
  }

  plUniquePtr<plModelImporter2::Importer> pImporter = plModelImporter2::RequestImporterForFileType(sAbsFilename);
  if (pImporter == nullptr)
    return plStatus("No known importer for this file type.");

  plModelImporter2::ImportOptions opt;
  opt.m_sSourceFile = sAbsFilename;
  opt.m_pAnimationOutput = &desc;
  opt.m_bAdditiveAnimation = pProp->m_bAdditive;
  opt.m_sAnimationToImport = pProp->m_sAnimationClipToExtract;
  opt.m_uiFirstAnimKeyframe = pProp->m_uiFirstFrame;
  opt.m_uiNumAnimKeyframes = pProp->m_uiNumFrames;

  const plResult res = pImporter->Import(opt);

  if (res.Succeeded())
  {
    if (pProp->m_RootMotionMode == plRootMotionSource::Constant)
    {
      desc.m_vConstantRootMotion = pProp->m_vConstantRootMotion;
    }

    range.BeginNextStep("Writing Result");

    pProp->m_EventTrack.ConvertToRuntimeData(desc.m_EventTrack);

    PL_SUCCEED_OR_RETURN(desc.Serialize(stream));
  }

  // if we found information about animation clips, update the UI, even if the transform failed
  if (!pImporter->m_OutputAnimationNames.IsEmpty())
  {
    pProp->m_AvailableClips.SetCount(pImporter->m_OutputAnimationNames.GetCount());
    for (plUInt32 clip = 0; clip < pImporter->m_OutputAnimationNames.GetCount(); ++clip)
    {
      pProp->m_AvailableClips[clip] = pImporter->m_OutputAnimationNames[clip];
    }

    // merge the new data with the actual asset document
    ApplyNativePropertyChangesToObjectManager(true);
  }

  if (res.Failed())
    return plStatus("Model importer was unable to read this asset.");

  return plStatus(PL_SUCCESS);
}

plTransformStatus plAnimationClipAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  // the preview mesh is an editor side only option, so the thumbnail context doesn't know anything about this
  // until we explicitly tell it about the mesh
  // without sending this here, thumbnails would remain black for assets transformed in the background
  if (!GetProperties()->m_sPreviewMesh.IsEmpty())
  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";
    msg.m_sPayload = GetProperties()->m_sPreviewMesh;
    SendMessageToEngine(&msg);
  }

  plStatus status = plAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}

plUuid plAnimationClipAssetDocument::InsertEventTrackCpAt(plInt64 iTickX, const char* szValue)
{
  plObjectCommandAccessor accessor(GetCommandHistory());
  plObjectAccessorBase& acc = accessor;
  acc.StartTransaction("Insert Event");

  const plAbstractProperty* pTrackProp = plGetStaticRTTI<plAnimationClipAssetProperties>()->FindPropertyByName("EventTrack");
  plUuid trackGuid = accessor.Get<plUuid>(GetPropertyObject(), pTrackProp);

  plUuid newObjectGuid;
  PL_VERIFY(
    acc.AddObject(accessor.GetObject(trackGuid), "ControlPoints", -1, plGetStaticRTTI<plEventTrackControlPointData>(), newObjectGuid).Succeeded(),
    "");
  const plDocumentObject* pCPObj = accessor.GetObject(newObjectGuid);
  PL_VERIFY(acc.SetValue(pCPObj, "Tick", iTickX).Succeeded(), "");
  PL_VERIFY(acc.SetValue(pCPObj, "Event", szValue).Succeeded(), "");

  acc.FinishTransaction();

  return newObjectGuid;
}

// void plAnimationClipAssetDocument::ApplyCustomRootMotion(plAnimationClipResourceDescriptor& anim) const
//{
//  const plAnimationClipAssetProperties* pProp = GetProperties();
//  const plUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
//  plArrayPtr<plTransform> pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);
//
//  const plVec3 vKeyframeMotion = pProp->m_vCustomRootMotion / (float)anim.GetFramesPerSecond();
//  const plTransform rootTransform(vKeyframeMotion);
//
//  for (plUInt32 kf = 0; kf < anim.GetNumFrames(); ++kf)
//  {
//    pRootTransforms[kf] = rootTransform;
//  }
//}
//
// void plAnimationClipAssetDocument::ExtractRootMotionFromFeet(plAnimationClipResourceDescriptor& anim, const plSkeleton& skeleton) const
//{
//  const plAnimationClipAssetProperties* pProp = GetProperties();
//  const plUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
//  plArrayPtr<plTransform> pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);
//
//  const plUInt16 uiFoot1 = skeleton.FindJointByName(plTempHashedString(pProp->m_sJoint1.GetData()));
//  const plUInt16 uiFoot2 = skeleton.FindJointByName(plTempHashedString(pProp->m_sJoint2.GetData()));
//
//  if (uiFoot1 == plInvalidJointIndex || uiFoot2 == plInvalidJointIndex)
//  {
//    plLog::Error("Joints '{0}' and '{1}' could not be found in animation clip", pProp->m_sJoint1, pProp->m_sJoint2);
//    return;
//  }
//
//  plAnimationPose pose;
//  pose.Configure(skeleton);
//
//  plVec3 lastFootPos1(0), lastFootPos2(0);
//
//  // init last foot position with very last frame data
//  {
//    pose.SetToBindPoseInLocalSpace(skeleton);
//    anim.SetPoseToKeyframe(pose, skeleton, anim.GetNumFrames() - 1);
//    pose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//
//    lastFootPos1 = pose.GetTransform(uiFoot1).GetTranslationVector();
//    lastFootPos2 = pose.GetTransform(uiFoot2).GetTranslationVector();
//  }
//
//  plInt32 lastFootDown = (lastFootPos1.z < lastFootPos2.z) ? 1 : 2;
//
//  plHybridArray<plUInt16, 32> unknownMotion;
//
//  for (plUInt16 frame = 0; frame < anim.GetNumFrames(); ++frame)
//  {
//    pose.SetToBindPoseInLocalSpace(skeleton);
//    anim.SetPoseToKeyframe(pose, skeleton, frame);
//    pose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//
//    const plVec3 footPos1 = pose.GetTransform(uiFoot1).GetTranslationVector();
//    const plVec3 footPos2 = pose.GetTransform(uiFoot2).GetTranslationVector();
//
//    const plVec3 footDir1 = footPos1 - lastFootPos1;
//    const plVec3 footDir2 = footPos2 - lastFootPos2;
//
//    plVec3 rootMotion(0);
//
//    const plInt32 curFootDown = (footPos1.z < footPos2.z) ? 1 : 2;
//
//    if (lastFootDown == curFootDown)
//    {
//      if (curFootDown == 1)
//        rootMotion = -footDir1;
//      else
//        rootMotion = -footDir2;
//
//      rootMotion.z = 0;
//      pRootTransforms[frame] = plTransform(rootMotion);
//    }
//    else
//    {
//      // set them via average later on
//      unknownMotion.PushBack(frame);
//      pRootTransforms[frame].SetIdentity();
//    }
//
//    lastFootDown = curFootDown;
//    lastFootPos1 = footPos1;
//    lastFootPos2 = footPos2;
//  }
//
//  // fix unknown motion frames
//  for (plUInt16 crossedFeet : unknownMotion)
//  {
//    const plUInt16 prevFrame = (crossedFeet > 0) ? (crossedFeet - 1) : anim.GetNumFrames() - 1;
//    const plUInt16 nextFrame = (crossedFeet + 1) % anim.GetNumFrames();
//
//    const plVec3 avgTranslation = plMath::Lerp(pRootTransforms[prevFrame].m_vPosition, pRootTransforms[nextFrame].m_vPosition, 0.5f);
//
//    pRootTransforms[crossedFeet] = plTransform(avgTranslation);
//  }
//
//  const plUInt16 numFrames = anim.GetNumFrames();
//
//  plHybridArray<plVec3, 32> translations;
//  translations.SetCount(numFrames);
//
//  for (plUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
//  {
//    translations[thisFrame] = pRootTransforms[thisFrame].m_vPosition;
//  }
//
//  // do some smoothing
//  for (plUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
//  {
//    const plUInt16 prevFrame2 = (numFrames + thisFrame - 2) % numFrames;
//    const plUInt16 prevFrame = (numFrames + thisFrame - 1) % numFrames;
//    const plUInt16 nextFrame = (thisFrame + 1) % numFrames;
//    const plUInt16 nextFrame2 = (thisFrame + 2) % numFrames;
//
//    const plVec3 smoothedTranslation =
//      (translations[prevFrame2] + translations[prevFrame] + translations[thisFrame] + translations[nextFrame] + translations[nextFrame2]) * 0.2f;
//
//    pRootTransforms[thisFrame].m_vPosition = smoothedTranslation;
//  }
//
//  // for (plUInt32 i = 0; i < anim.GetNumFrames(); ++i)
//  //{
//  //  plLog::Info("Motion {0}: {1} | {2}", plArgI(i, 3), plArgF(pRootTransforms[i].m_vPosition.x, 1),
//  //              plArgF(pRootTransforms[i].m_vPosition.y, 1));
//  //}
//}
//
// void plAnimationClipAssetDocument::MakeRootMotionConstantAverage(plAnimationClipResourceDescriptor& anim) const
//{
//  const plUInt16 uiRootMotionJointIdx = anim.GetRootMotionJoint();
//  plArrayPtr<plTransform> pRootTransforms = anim.GetJointKeyframes(uiRootMotionJointIdx);
//  const plUInt16 numFrames = anim.GetNumFrames();
//
//  plVec3 avgFootTranslation(0);
//
//  for (plUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
//  {
//    avgFootTranslation += pRootTransforms[thisFrame].m_vPosition;
//  }
//
//  avgFootTranslation /= numFrames;
//
//  for (plUInt16 thisFrame = 0; thisFrame < numFrames; ++thisFrame)
//  {
//    pRootTransforms[thisFrame].m_vPosition = avgFootTranslation;
//  }
//}

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plAnimationClipAssetDocumentGenerator, 1, plRTTIDefaultAllocator<plAnimationClipAssetDocumentGenerator>)
PL_END_DYNAMIC_REFLECTED_TYPE;

plAnimationClipAssetDocumentGenerator::plAnimationClipAssetDocumentGenerator()
{
  AddSupportedFileType("fbx");
  AddSupportedFileType("gltf");
  AddSupportedFileType("glb");
}

plAnimationClipAssetDocumentGenerator::~plAnimationClipAssetDocumentGenerator() = default;

void plAnimationClipAssetDocumentGenerator::GetImportModes(plStringView sAbsInputFile, plDynamicArray<plAssetDocumentGenerator::ImportMode>& out_modes) const
{
  {
    plAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = plAssetDocGeneratorPriority::Undecided;
    info.m_sName = "AnimationClipImport_Single";
    info.m_sIcon = ":/AssetIcons/Animation_Clip.svg";
  }

  {
    plAssetDocumentGenerator::ImportMode& info = out_modes.ExpandAndGetRef();
    info.m_Priority = plAssetDocGeneratorPriority::Undecided;
    info.m_sName = "AnimationClipImport_All";
    info.m_sIcon = ":/AssetIcons/Animation_Clip.svg";
  }
}

plStatus plAnimationClipAssetDocumentGenerator::Generate(plStringView sInputFileAbs, plStringView sMode, plDynamicArray<plDocument*>& out_generatedDocuments)
{
  plStringBuilder sOutFile = sInputFileAbs;
  sOutFile.ChangeFileExtension(GetDocumentExtension());
  plOSFile::FindFreeFilename(sOutFile);

  auto pApp = plQtEditorApp::GetSingleton();

  plStringBuilder sInputFileRel = sInputFileAbs;
  pApp->MakePathDataDirectoryRelative(sInputFileRel);

  plStringBuilder title;
  title.SetFormat("Select Preview Mesh for Animation Clip '{}'", sInputFileAbs.GetFileName());

  plStringBuilder sPreviewMesh;

  plQtAssetBrowserDlg dlg(nullptr, plUuid::MakeInvalid(), "CompatibleAsset_Mesh_Skinned", title);
  if (dlg.exec() != 0)
  {
    if (dlg.GetSelectedAssetGuid().IsValid())
    {
      plConversionUtils::ToString(dlg.GetSelectedAssetGuid(), sPreviewMesh);
    }
  }

  if (sMode == "AnimationClipImport_Single")
  {
    plDocument* pDoc = pApp->CreateDocument(sOutFile, plDocumentFlags::None);
    if (pDoc == nullptr)
      return plStatus("Could not create target document");

    out_generatedDocuments.PushBack(pDoc);

    plAnimationClipAssetDocument* pAssetDoc = plDynamicCast<plAnimationClipAssetDocument*>(pDoc);

    auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
    accessor.SetValue("File", sInputFileRel.GetView());
    accessor.SetValue("PreviewMesh", sPreviewMesh.GetView());

    return plStatus(PL_SUCCESS);
  }

  if (sMode == "AnimationClipImport_All")
  {
    plModelImporter2::ImportOptions opt;
    opt.m_sSourceFile = sInputFileAbs;

    plUniquePtr<plModelImporter2::Importer> pImporter = plModelImporter2::RequestImporterForFileType(opt.m_sSourceFile);
    if (pImporter == nullptr)
      return plStatus("No known importer for this file type.");

    if (pImporter->Import(opt).Failed())
      return plStatus("Failed to import asset.");

    plStringBuilder sFilename;
    plStringBuilder sOutFile2;

    for (const auto& clip : pImporter->m_OutputAnimationNames)
    {
      sFilename = clip;
      sFilename.ReplaceAll(" ", "-");
      sFilename.Prepend(sOutFile.GetFileName(), "_");

      sOutFile2 = sOutFile;
      sOutFile2.ChangeFileName(sFilename);
      plOSFile::FindFreeFilename(sOutFile2);

      plDocument* pDoc = pApp->CreateDocument(sOutFile2, plDocumentFlags::None);
      if (pDoc == nullptr)
        return plStatus("Could not create target document");

      out_generatedDocuments.PushBack(pDoc);

      plAnimationClipAssetDocument* pAssetDoc = plDynamicCast<plAnimationClipAssetDocument*>(pDoc);

      auto& accessor = pAssetDoc->GetPropertyObject()->GetTypeAccessor();
      accessor.SetValue("File", sInputFileRel.GetView());
      accessor.SetValue("UseAnimationClip", clip);
      accessor.SetValue("PreviewMesh", sPreviewMesh.GetView());
    }

    return plStatus(PL_SUCCESS);
  }

  PL_ASSERT_NOT_IMPLEMENTED;
  return plStatus(PL_FAILURE);
}
