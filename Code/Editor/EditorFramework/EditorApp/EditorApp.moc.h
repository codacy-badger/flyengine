#pragma once

#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorFramework/EditorApp/Configuration/Plugins.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/ContainerWindow/ContainerWindow.moc.h>
#include <QApplication>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Utilities/RecentFilesList.h>

class QMainWindow;
class QWidget;
class plProgress;
class plQtProgressbar;
class plQtEditorApp;
template <typename T>
class QList;
using QStringList = QList<QString>;
class plTranslatorFromFiles;
class plDynamicStringEnum;
class QSplashScreen;
class plQtVersionChecker;

struct PL_EDITORFRAMEWORK_DLL plEditorAppEvent
{
  enum class Type
  {
    BeforeApplyDataDirectories, ///< Sent after data directory config was loaded, but before it is applied. Allows to add custom
                                ///< dependencies at the right moment.
    ReloadResources,            ///< Sent when 'ReloadResources' has been triggered (and a message was sent to the engine)
    EditorStarted,              ///< Editor has finished all initialization code and will now load the recent project.
  };

  Type m_Type;
};

class PL_EDITORFRAMEWORK_DLL plQtEditorApp : public QObject
{
  Q_OBJECT

  PL_DECLARE_SINGLETON(plQtEditorApp);

public:
  struct StartupFlags
  {
    using StorageType = plUInt8;
    enum Enum
    {
      Headless = PL_BIT(0),   ///< The app does not do any rendering.
      SafeMode = PL_BIT(1),   ///< '-safe' : Prevent automatic loading of projects, scenes, etc. to minimize risk of crashing.
      NoRecent = PL_BIT(2),   ///< '-norecent' : Do not modify recent file lists. Used for modes such as tests, where the user does not do any interactions.
      UnitTest = PL_BIT(3),   ///< Specified when the process is running as a unit test
      Background = PL_BIT(4), ///< This process is an editor processor background process handling IPC tasks of the editor parent process.
      Default = 0,
    };

    struct Bits
    {
      StorageType Headless : 1;
      StorageType SafeMode : 1;
      StorageType NoRecent : 1;
      StorageType UnitTest : 1;
      StorageType Background : 1;
    };
  };

public:
  plQtEditorApp();
  ~plQtEditorApp();

  static plEvent<const plEditorAppEvent&> m_Events;

  //
  // External Tools
  //

  /// \brief Searches for an external tool.
  ///
  /// Either uses one from the precompiled tools folder, or from the currently compiled binaries, depending where it finds one.
  /// If the editor preference is set to use precompiled tools, that folder is preferred, otherwise the other folder is preferred.
  plString FindToolApplication(const char* szToolName);

  /// \brief Executes an external tool as found by FindToolApplication().
  ///
  /// The applications output is parsed and forwarded to the given log interface. A custom log level is applied first.
  /// If the tool cannot be found or it takes longer to execute than the allowed timeout, the function returns failure.
  plStatus ExecuteTool(const char* szTool, const QStringList& arguments, plUInt32 uiSecondsTillTimeout, plLogInterface* pLogOutput = nullptr, plLogMsgType::Enum logLevel = plLogMsgType::WarningMsg, const char* szCWD = nullptr);

  /// \brief Creates the string with which to run Fileserve for the currently open project.
  plString BuildFileserveCommandLine() const;

  /// \brief Launches Fileserve with the settings for the current project.
  void RunFileserve();

  /// \brief Launches plInspector.
  void RunInspector();

  //
  //
  //

  /// \brief Can be set via the command line option '-safe'. In this mode the editor will not automatically load recent documents
  bool IsInSafeMode() const { return m_StartupFlags.IsSet(StartupFlags::SafeMode); }

  /// \brief Returns true if the the app shouldn't display anything. This is the case in an EditorProcessor.
  bool IsInHeadlessMode() const { return m_StartupFlags.IsSet(StartupFlags::Headless); }

  /// \brief Returns true if the editor is started in run in test mode.
  bool IsInUnitTestMode() const { return m_StartupFlags.IsSet(StartupFlags::UnitTest); }

  /// \brief Returns true if the editor is started in run in background mode.
  bool IsBackgroundMode() const { return m_StartupFlags.IsSet(StartupFlags::Background); }

  const plPluginBundleSet& GetPluginBundles() const { return m_PluginBundles; }
  plPluginBundleSet& GetPluginBundles() { return m_PluginBundles; }

  void AddRestartRequiredReason(const char* szReason);
  const plSet<plString>& GetRestartRequiredReasons() { return m_RestartRequiredReasons; }

  void AddReloadProjectRequiredReason(const char* szReason);
  const plSet<plString>& GetReloadProjectRequiredReason() { return m_ReloadProjectRequiredReasons; }

  void SaveSettings();

  /// \brief Writes a file containing all the currently open documents
  void SaveOpenDocumentsList();

  /// \brief Reads the list of last open documents in the current project.
  plRecentFilesList LoadOpenDocumentsList();

  void InitQt(int iArgc, char** pArgv);
  void StartupEditor();
  void StartupEditor(plBitflags<StartupFlags> startupFlags, const char* szUserDataFolder = nullptr);
  void ShutdownEditor();
  plInt32 RunEditor();
  void DeInitQt();

  void LoadEditorPlugins();

  plRecentFilesList& GetRecentProjectsList() { return m_RecentProjects; }
  plRecentFilesList& GetRecentDocumentsList() { return m_RecentDocuments; }

  plEditorEngineProcessConnection* GetEngineViewProcess() { return m_pEngineViewProcess; }

  void ShowSettingsDocument();
  void CloseSettingsDocument();

  void CloseProject();
  plResult OpenProject(const char* szProject, bool bImmediate = false);

  void GuiCreateDocument();
  void GuiOpenDocument();

  void GuiOpenDashboard();
  void GuiOpenDocsAndCommunity();
  bool GuiCreateProject(bool bImmediate = false);
  bool GuiOpenProject(bool bImmediate = false);

  void OpenDocumentQueued(plStringView sDocument, const plDocumentObject* pOpenContext = nullptr);
  plDocument* OpenDocument(plStringView sDocument, plBitflags<plDocumentFlags> flags, const plDocumentObject* pOpenContext = nullptr);
  plDocument* CreateDocument(plStringView sDocument, plBitflags<plDocumentFlags> flags, const plDocumentObject* pOpenContext = nullptr);

  plResult CreateOrOpenProject(bool bCreate, plStringView sFile);

  /// \brief If this project is remote, ie coming from another repository that is not checked-out by default, make sure it exists locally on disk.
  ///
  /// Adjusts inout_sFilePath from pointing to an plRemoteProject file to a plProject file, if necessary.
  /// If the project is already local, it always succeeds.
  /// If checking out fails or is user canceled, the function returns failure.
  plStatus MakeRemoteProjectLocal(plStringBuilder& inout_sFilePath);

  bool ExistsPluginSelectionStateDDL(const char* szProjectDir = ":project");
  void WritePluginSelectionStateDDL(const char* szProjectDir = ":project");
  void CreatePluginSelectionDDL(const char* szProjectFile, const char* szTemplate);
  void LoadPluginBundleDlls(const char* szProjectFile);
  void DetectAvailablePluginBundles(plStringView sSearchDirectory);

  /// \brief Launches a new instance of the editor to open the given project.
  void LaunchEditor(const char* szProject, bool bCreate);

  /// \brief Adds a data directory as a hard dependency to the project. Should be used by plugins to ensure their required data is
  /// available. The path must be relative to the SdkRoot folder.
  void AddPluginDataDirDependency(const char* szSdkRootRelativePath, const char* szRootName = nullptr, bool bWriteable = false);

  const plApplicationFileSystemConfig& GetFileSystemConfig() const { return m_FileSystemConfig; }
  const plApplicationPluginConfig GetRuntimePluginConfig(bool bIncludeEditorPlugins) const;

  void SetFileSystemConfig(const plApplicationFileSystemConfig& cfg);

  bool MakeDataDirectoryRelativePathAbsolute(plStringBuilder& ref_sPath) const;
  bool MakeDataDirectoryRelativePathAbsolute(plString& ref_sPath) const;
  bool MakePathDataDirectoryRelative(plStringBuilder& ref_sPath) const;
  bool MakePathDataDirectoryRelative(plString& ref_sPath) const;

  bool MakePathDataDirectoryParentRelative(plStringBuilder& ref_sPath) const;
  bool MakeParentDataDirectoryRelativePathAbsolute(plStringBuilder& ref_sPath, bool bCheckExists) const;

  plStatus SaveTagRegistry();

  /// \brief Reads the known input slots from disk and adds them to the existing list.
  ///
  /// All input slots to be exposed by the editor are stored in 'Shared/Tools/plEditor/InputSlots'
  /// as txt files. Each line names one input slot.
  void GetKnownInputSlots(plDynamicArray<plString>& slots) const;

  /// \brief Instructs the engine to reload its resources
  void ReloadEngineResources();

  void RestartEngineProcessIfPluginsChanged(bool bForce);
  void SetStyleSheet();

Q_SIGNALS:
  void IdleEvent();

private:
  plString BuildDocumentTypeFileFilter(bool bForCreation);

  void GuiCreateOrOpenDocument(bool bCreate);
  bool GuiCreateOrOpenProject(bool bCreate);

private Q_SLOTS:
  void SlotTimedUpdate();
  void SlotQueuedCloseProject();
  void SlotQueuedOpenProject(QString sProject);
  void SlotQueuedOpenDocument(QString sProject, void* pOpenContext);
  void SlotQueuedGuiOpenDashboard();
  void SlotQueuedGuiOpenDocsAndCommunity();
  void SlotQueuedGuiCreateOrOpenProject(bool bCreate);
  void SlotSaveSettings();
  void SlotVersionCheckCompleted(bool bNewVersionReleased, bool bForced);

private:
  void UpdateGlobalStatusBarMessage();

  void DocumentManagerRequestHandler(plDocumentManager::Request& r);
  void DocumentManagerEventHandler(const plDocumentManager::Event& r);
  void DocumentEventHandler(const plDocumentEvent& e);
  void DocumentWindowEventHandler(const plQtDocumentWindowEvent& e);
  void ProjectRequestHandler(plToolsProjectRequest& r);
  void ProjectEventHandler(const plToolsProjectEvent& r);
  void EngineProcessMsgHandler(const plEditorEngineProcessConnection::Event& e);
  void UiServicesEvents(const plQtUiServices::Event& e);

  void SetupNewProject();
  void LoadEditorPreferences();
  void LoadProjectPreferences();
  void StoreEnginePluginModificationTimes();
  bool CheckForEnginePluginModifications();
  void SaveAllOpenDocuments();

  void ReadTagRegistry();

  void SetupDataDirectories();
  void CreatePanels();

  void SetupAndShowSplashScreen();
  void CloseSplashScreen();

  plResult AddBundlesInOrder(plDynamicArray<plApplicationPluginConfig::PluginConfig>& order, const plPluginBundleSet& bundles, const plString& start, bool bEditor, bool bEditorEngine, bool bRuntime) const;

  bool m_bSavePreferencesAfterOpenProject;
  bool m_bLoadingProjectInProgress = false;
  bool m_bAnyProjectOpened = false;
  bool m_bWroteCrashIndicatorFile = false;

  plBitflags<StartupFlags> m_StartupFlags;
  plDynamicArray<plString> m_DocumentsToOpen;

  plSet<plString> m_RestartRequiredReasons;
  plSet<plString> m_ReloadProjectRequiredReasons;

  plPluginBundleSet m_PluginBundles;

  void SaveRecentFiles();
  void LoadRecentFiles();

  plRecentFilesList m_RecentProjects;
  plRecentFilesList m_RecentDocuments;

  int m_iArgc = 0;
  QApplication* m_pQtApplication = nullptr;
  plLongOpControllerManager m_LongOpControllerManager;
  plEditorEngineProcessConnection* m_pEngineViewProcess;
  QTimer* m_pTimer = nullptr;

  QSplashScreen* m_pSplashScreen = nullptr;

  plLogWriter::HTML m_LogHTML;

  plTime m_LastPluginModificationCheck;
  plApplicationFileSystemConfig m_FileSystemConfig;

  // *** Recent Paths ***
  plString m_sLastDocumentFolder;
  plString m_sLastProjectFolder;

  // *** Progress Bar ***
public:
  bool IsProgressBarProcessingEvents() const;

private:
  plProgress* m_pProgressbar = nullptr;
  plQtProgressbar* m_pQtProgressbar = nullptr;

  // *** Localization ***
  plTranslatorFromFiles* m_pTranslatorFromFiles = nullptr;

  // *** Dynamic Enum Strings ***
  plSet<plString> m_DynamicEnumStringsToClear;
  void OnDemandDynamicStringEnumLoad(plStringView sEnumName, plDynamicStringEnum& e);

  plUniquePtr<plQtVersionChecker> m_pVersionChecker;
};

PL_DECLARE_FLAGS_OPERATORS(plQtEditorApp::StartupFlags);
