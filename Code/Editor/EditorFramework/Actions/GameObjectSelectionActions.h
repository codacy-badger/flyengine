#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plGameObjectDocument;

///
class PL_EDITORFRAMEWORK_DLL plGameObjectSelectionActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(plStringView sMapping);
  static void MapContextMenuActions(plStringView sMapping);
  static void MapViewContextMenuActions(plStringView sMapping);

  static plActionDescriptorHandle s_hSelectionCategory;
  static plActionDescriptorHandle s_hShowInScenegraph;
  static plActionDescriptorHandle s_hFocusOnSelection;
  static plActionDescriptorHandle s_hFocusOnSelectionAllViews;
  static plActionDescriptorHandle s_hSnapCameraToObject;
  static plActionDescriptorHandle s_hMoveCameraHere;
  static plActionDescriptorHandle s_hCreateEmptyGameObjectHere;
};

///
class PL_EDITORFRAMEWORK_DLL plGameObjectSelectionAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plGameObjectSelectionAction, plButtonAction);

public:
  enum class ActionType
  {
    ShowInScenegraph,
    FocusOnSelection,
    FocusOnSelectionAllViews,
    SnapCameraToObject,
    MoveCameraHere,
    CreateGameObjectHere,
  };

  plGameObjectSelectionAction(const plActionContext& context, const char* szName, ActionType type);
  ~plGameObjectSelectionAction();

  virtual void Execute(const plVariant& value) override;

private:
  void SelectionEventHandler(const plSelectionManagerEvent& e);

  void UpdateEnableState();

  plGameObjectDocument* m_pSceneDocument;
  ActionType m_Type;
};
