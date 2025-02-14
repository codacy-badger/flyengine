#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/ui_RemoteConnectionDlg.h>
#include <Foundation/Strings/String.h>
#include <QDialog>

class PL_EDITORFRAMEWORK_DLL plQtRemoteConnectionDlg : public QDialog, public Ui_plQtRemoteConnectionDlg
{
public:
  Q_OBJECT

  struct Address
  {
    plUInt8 part[4];

    Address();
    void operator=(const Address& rhs);
    bool operator==(const Address& rhs) const;
    bool IsEmpty() const;
  };

public:
  plQtRemoteConnectionDlg(QWidget* pParent);
  ~plQtRemoteConnectionDlg();

  Address m_UsedAddress;
  Address m_UsedFsAddress;

  QString GetResultingAddress() const;
  QString GetResultingFsAddress() const;

private Q_SLOTS:
  void on_ButtonConnect_clicked();
  void on_ButtonLaunchFS_clicked();
  void onRecentIPselected();
  void onRecentFsIPselected();

private:
  Address m_RecentAddresses[5];
  Address m_RecentFsAddresses[5];

  virtual void showEvent(QShowEvent* event) override;
  void AddToRecentAddresses(Address* pRecentAddresses, const Address& addr);
  void SetCurrentIP(const Address& addr);
  void SetCurrentFsIP(const Address& addr);
};

