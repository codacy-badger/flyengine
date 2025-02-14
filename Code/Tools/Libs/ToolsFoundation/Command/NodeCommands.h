#pragma once

#include <ToolsFoundation/Command/Command.h>

class plDocumentObject;
class plCommandHistory;
class plPin;

class PL_TOOLSFOUNDATION_DLL plRemoveNodeCommand : public plCommand
{
  PL_ADD_DYNAMIC_REFLECTION(plRemoveNodeCommand, plCommand);

public:
  plRemoveNodeCommand();

public: // Properties
  plUuid m_Object;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override;

private:
  plDocumentObject* m_pObject = nullptr;
};


class PL_TOOLSFOUNDATION_DLL plMoveNodeCommand : public plCommand
{
  PL_ADD_DYNAMIC_REFLECTION(plMoveNodeCommand, plCommand);

public:
  plMoveNodeCommand();

public: // Properties
  plUuid m_Object;
  plVec2 m_NewPos = plVec2::MakeZero();

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  plDocumentObject* m_pObject = nullptr;
  plVec2 m_vOldPos = plVec2::MakeZero();
};


class PL_TOOLSFOUNDATION_DLL plConnectNodePinsCommand : public plCommand
{
  PL_ADD_DYNAMIC_REFLECTION(plConnectNodePinsCommand, plCommand);

public:
  plConnectNodePinsCommand();

public: // Properties
  plUuid m_ConnectionObject;
  plUuid m_ObjectSource;
  plUuid m_ObjectTarget;
  plString m_sSourcePin;
  plString m_sTargetPin;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  plDocumentObject* m_pConnectionObject = nullptr;
  plDocumentObject* m_pObjectSource = nullptr;
  plDocumentObject* m_pObjectTarget = nullptr;
};


class PL_TOOLSFOUNDATION_DLL plDisconnectNodePinsCommand : public plCommand
{
  PL_ADD_DYNAMIC_REFLECTION(plDisconnectNodePinsCommand, plCommand);

public:
  plDisconnectNodePinsCommand();

public: // Properties
  plUuid m_ConnectionObject;

private:
  virtual plStatus DoInternal(bool bRedo) override;
  virtual plStatus UndoInternal(bool bFireEvents) override;
  virtual void CleanupInternal(CommandState state) override {}

private:
  plDocumentObject* m_pConnectionObject = nullptr;
  const plDocumentObject* m_pObjectSource = nullptr;
  const plDocumentObject* m_pObjectTarget = nullptr;
  plString m_sSourcePin;
  plString m_sTargetPin;
};


class PL_TOOLSFOUNDATION_DLL plNodeCommands
{
public:
  static plStatus AddAndConnectCommand(plCommandHistory* pHistory, const plRTTI* pConnectionType, const plPin& sourcePin, const plPin& targetPin);
  static plStatus DisconnectAndRemoveCommand(plCommandHistory* pHistory, const plUuid& connectionObject);
};
