#include <EditorPluginJolt/EditorPluginJoltPCH.h>

#include <EditorFramework/Actions/AssetActions.h>
#include <EditorFramework/Actions/CommonAssetActions.h>
#include <EditorFramework/Actions/ProjectActions.h>
#include <EditorPluginJolt/Actions/JoltActions.h>
#include <EditorPluginJolt/CollisionMeshAsset/JoltCollisionMeshAssetObjects.h>
#include <EditorPluginJolt/Dialogs/JoltProjectSettingsDlg.moc.h>
#include <GameEngine/Physics/CollisionFilter.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/CommandHistoryActions.h>
#include <GuiFoundation/Action/DocumentActions.h>
#include <GuiFoundation/Action/StandardMenus.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>

void UpdateCollisionLayerDynamicEnumValues();

static void ToolsProjectEventHandler(const plToolsProjectEvent& e);

void OnLoadPlugin()
{
  plToolsProject::GetSingleton()->s_Events.AddEventHandler(ToolsProjectEventHandler);

  // Collision Mesh
  {
    plPropertyMetaState::GetSingleton()->m_Events.AddEventHandler(plJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler);

    // Menu Bar
    {
      plActionMapManager::RegisterActionMap("JoltCollisionMeshAssetMenuBar").IgnoreResult();
      plStandardMenus::MapActions("JoltCollisionMeshAssetMenuBar", plStandardMenuTypes::Default | plStandardMenuTypes::Edit);
      plProjectActions::MapActions("JoltCollisionMeshAssetMenuBar");
      plDocumentActions::MapMenuActions("JoltCollisionMeshAssetMenuBar");
      plAssetActions::MapMenuActions("JoltCollisionMeshAssetMenuBar");
      plCommandHistoryActions::MapActions("JoltCollisionMeshAssetMenuBar");
    }

    // Tool Bar
    {
      plActionMapManager::RegisterActionMap("JoltCollisionMeshAssetToolBar").IgnoreResult();
      plDocumentActions::MapToolbarActions("JoltCollisionMeshAssetToolBar");
      plCommandHistoryActions::MapActions("JoltCollisionMeshAssetToolBar", "");
      plAssetActions::MapToolBarActions("JoltCollisionMeshAssetToolBar", true);
      plCommonAssetActions::MapToolbarActions("JoltCollisionMeshAssetToolBar", plCommonAssetUiState::Grid);
    }
  }

  // Scene
  {
    // Menu Bar
    {
      plJoltActions::RegisterActions();
      plJoltActions::MapMenuActions();
    }

    // Tool Bar
    {
    }
  }
}

void OnUnloadPlugin()
{
  plJoltActions::UnregisterActions();
  plToolsProject::GetSingleton()->s_Events.RemoveEventHandler(ToolsProjectEventHandler);
  plPropertyMetaState::GetSingleton()->m_Events.RemoveEventHandler(plJoltCollisionMeshAssetProperties::PropertyMetaStateEventHandler);
}

PL_PLUGIN_ON_LOADED()
{
  OnLoadPlugin();
}

PL_PLUGIN_ON_UNLOADED()
{
  OnUnloadPlugin();
}

void UpdateCollisionLayerDynamicEnumValues()
{
  auto& cfe = plDynamicEnum::GetDynamicEnum("PhysicsCollisionLayer");
  cfe.Clear();

  plCollisionFilterConfig cfg;
  if (cfg.Load().Failed())
  {
    return;
  }

  // add all names and values that are valid (non-empty)
  for (plInt32 i = 0; i < 32; ++i)
  {
    if (!cfg.GetGroupName(i).IsEmpty())
    {
      cfe.SetValueAndName(i, cfg.GetGroupName(i));
    }
  }
}

static void ToolsProjectEventHandler(const plToolsProjectEvent& e)
{
  if (e.m_Type == plToolsProjectEvent::Type::ProjectSaveState)
  {
    plQtJoltProjectSettingsDlg::EnsureConfigFileExists();
  }

  if (e.m_Type == plToolsProjectEvent::Type::ProjectOpened)
  {
    UpdateCollisionLayerDynamicEnumValues();
  }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plJoltRopeComponentPatch_1_2 : public plGraphPatch
{
public:
  plJoltRopeComponentPatch_1_2()
    : plGraphPatch("plJoltRopeComponent", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Anchor", "Anchor2");
    pNode->RenameProperty("AttachToOrigin", "AttachToAnchor1");
    pNode->RenameProperty("AttachToAnchor", "AttachToAnchor2");
  }
};

plJoltRopeComponentPatch_1_2 g_plJoltRopeComponentPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class plJoltHitboxComponentPatch_1_2 : public plGraphPatch
{
public:
  plJoltHitboxComponentPatch_1_2()
    : plGraphPatch("plJoltBoneColliderComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    ref_context.RenameClass("plJoltHitboxComponent");
  }
};

plJoltHitboxComponentPatch_1_2 g_plJoltHitboxComponentPatch_1_2;
