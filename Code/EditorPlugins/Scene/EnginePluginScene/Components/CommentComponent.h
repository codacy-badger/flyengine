#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <EditorEngineProcessFramework/SceneExport/SceneExportModifier.h>

using plCommentComponentManager = plComponentManager<class plCommentComponent, plBlockStorageType::Compact>;

/// \brief This component is for adding notes to objects in a scene.
///
/// These comments are solely to explain things to other people that look at the scene or prefab structure.
/// They are not meant for use at runtime. Therefore, all instances of plCommentComponent are automatically stripped from a scene during export.
class PL_ENGINEPLUGINSCENE_DLL plCommentComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plCommentComponent, plComponent, plCommentComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plCommentComponent

public:
  plCommentComponent();
  ~plCommentComponent();

  void SetComment(const char* szText);
  const char* GetComment() const;

private:
  plHashedString m_sComment;
};

//////////////////////////////////////////////////////////////////////////

class PL_ENGINEPLUGINSCENE_DLL plSceneExportModifier_RemoveCommentComponents : public plSceneExportModifier
{
  PL_ADD_DYNAMIC_REFLECTION(plSceneExportModifier_RemoveCommentComponents, plSceneExportModifier);

public:
  virtual void ModifyWorld(plWorld& ref_world, plStringView sDocumentType, const plUuid& documentGuid, bool bForExport) override;
};
