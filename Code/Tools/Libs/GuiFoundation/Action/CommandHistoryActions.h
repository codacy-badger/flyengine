#pragma once

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>
#include <ToolsFoundation/CommandHistory/CommandHistory.h>

///
class PL_GUIFOUNDATION_DLL plCommandHistoryActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(plStringView sMapping, plStringView sTargetMenu = "G.Edit");

  static plActionDescriptorHandle s_hCommandHistoryCategory;
  static plActionDescriptorHandle s_hUndo;
  static plActionDescriptorHandle s_hRedo;
};


///
class PL_GUIFOUNDATION_DLL plCommandHistoryAction : public plDynamicActionAndMenuAction
{
  PL_ADD_DYNAMIC_REFLECTION(plCommandHistoryAction, plDynamicActionAndMenuAction);

public:
  enum class ButtonType
  {
    Undo,
    Redo,
  };

  plCommandHistoryAction(const plActionContext& context, const char* szName, ButtonType button);
  ~plCommandHistoryAction();

  virtual void Execute(const plVariant& value) override;
  virtual void GetEntries(plHybridArray<plDynamicMenuAction::Item, 16>& out_entries) override;

private:
  void UpdateState();
  void CommandHistoryEventHandler(const plCommandHistoryEvent& e);

  ButtonType m_ButtonType;
};
