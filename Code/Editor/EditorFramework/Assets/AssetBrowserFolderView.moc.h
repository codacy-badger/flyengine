#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <ToolsFoundation/FileSystem/FileSystemModel.h>
#include <ToolsFoundation/Project/ToolsProject.h>

#include <QItemDelegate>
#include <QTreeWidget>
#include <QValidator>

class plQtAssetBrowserFilter;

/// \brief Basic file name validator. Makes sure that under a given parent folder, the new file name is valid and not already in use by a different file.
class plFileNameValidator : public QValidator
{
public:
  /// \brief Constructor. Validator requires the current location and name of the file.
  /// \param sParentFolder Absolute path to the location of the file.
  /// \param sCurrentName Current filename. If set, this name is marked as valid, even though it is already in use.
  plFileNameValidator(QObject* pParent, plStringView sParentFolder, plStringView sCurrentName);
  virtual QValidator::State validate(QString& ref_sInput, int& ref_iPos) const override;

private:
  plString m_sParentFolder;
  plString m_sCurrentName;
};

/// \brief Custom delegate for the eqQtAssetBrowserFolderView to enable renaming folders. Does not do any model modifications. Instead, it fires editingFinished when the delegate editor closes.
class plFolderNameDelegate : public QItemDelegate
{
  Q_OBJECT

public:
  plFolderNameDelegate(QObject* pParent = nullptr);

  virtual QWidget* createEditor(QWidget* pParent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
  virtual void setModelData(QWidget* pEditor, QAbstractItemModel* pModel, const QModelIndex& index) const override;

signals:
  void editingFinished(const QString& sAbsPath, const QString& sNewName) const;
};

/// \brief Folder tree of the asset browser to allow filtering by folder.
///
/// This class keeps up to date with the folder structure in plFileSystemModel. Events from plFileSystemModel are cached ans flushed via OnFlushFileSystemEvents.
/// Folder movement, creation and deletion is supported and handled by this class. The context menu is implemented in plQtAssetBrowserWidget as it requires a global context of the asset browser instance.
class eqQtAssetBrowserFolderView : public QTreeWidget
{
  Q_OBJECT
public:
  eqQtAssetBrowserFolderView(QWidget* pParent);
  ~eqQtAssetBrowserFolderView();

  /// \brief Required to be set right after the ctor. This class will call plQtAssetBrowserFilter::SetPathFilter whenever the current selected item changes.
  void SetFilter(plQtAssetBrowserFilter* pFilter);
  /// \brief In dialog mode, any modifications (folder movement, creation and deletion) are disabled.
  void SetDialogMode(bool bDialogMode);

  virtual void mousePressEvent(QMouseEvent* e) override;

public Q_SLOTS:
  /// \brief Creates a new folder under the current selected item and enters edit mode to allow the user to rename it.
  void NewFolder();
  /// \brief Opens the current selected item in the windows explorer or OS equivalent.
  void TreeOpenExplorer();
  /// \brief Deletes the currently selected folder after confirmation.
  void DeleteFolder();

private Q_SLOTS:
  void OnFolderEditingFinished(const QString& sAbsPath, const QString& sNewName);
  void OnFlushFileSystemEvents();
  void OnItemSelectionChanged();
  void OnPathFilterChanged();

protected:
  virtual void dragMoveEvent(QDragMoveEvent* e) override;
  virtual void dropEvent(QDropEvent* event) override;
  virtual Qt::DropActions supportedDropActions() const override;
  plStatus canDrop(QDropEvent* e, plDynamicArray<plString>& out_files, plString& out_sTargetFolder);
  virtual QStringList mimeTypes() const override;
  virtual QMimeData* mimeData(const QList<QTreeWidgetItem*>& items) const override;
  virtual void keyPressEvent(QKeyEvent* e) override;

private:
  bool SelectPathFilter(QTreeWidgetItem* pParent, const QString& sPath);
  void UpdateDirectoryTree();
  void ClearDirectoryTree();
  void BuildDirectoryTree(const plDataDirPath& path, plStringView sCurPath, QTreeWidgetItem* pParent, plStringView sCurPathToItem, bool bIsHidden);
  void RemoveDirectoryTreeItem(plStringView sCurPath, QTreeWidgetItem* pParent, plStringView sCurPathToItem);
  QTreeWidgetItem* FindDirectoryTreeItem(plStringView sCurPath, QTreeWidgetItem* pParent, plStringView sCurPathToItem);
  void FileSystemModelFolderEventHandler(const plFolderChangedEvent& e);
  void ProjectEventHandler(const plToolsProjectEvent& e);

private:
  bool m_bDialogMode = false;
  plUInt32 m_uiKnownAssetFolderCount = 0;
  bool m_bTreeSelectionChangeInProgress = false;

  plQtAssetBrowserFilter* m_pFilter = nullptr;

  plMutex m_FolderStructureMutex;
  plHybridArray<plFolderChangedEvent, 2> m_QueuedFolderEvents;
};
