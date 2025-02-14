#pragma once

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <EditorFramework/EditorFrameworkDLL.h>

template <typename T>
class QList;
class QString;
using QStringList = QList<QString>;
class QProcess;

class PL_EDITORFRAMEWORK_DLL plEditorProcessCommunicationChannel : public plProcessCommunicationChannel
{
public:
  plResult StartClientProcess(const char* szProcess, const QStringList& args, bool bRemote, const plRTTI* pFirstAllowedMessageType = nullptr,
    plUInt32 uiMemSize = 1024 * 1024 * 10);

  bool IsClientAlive() const;

  void CloseConnection();

  plString GetStdoutContents();

private:
  QProcess* m_pClientProcess = nullptr;
};

class PL_EDITORFRAMEWORK_DLL plEditorProcessRemoteCommunicationChannel : public plProcessCommunicationChannel
{
public:
  plResult ConnectToServer(const char* szAddress);

  bool IsConnected() const;

  void CloseConnection();

  void TryConnect();

private:
};
