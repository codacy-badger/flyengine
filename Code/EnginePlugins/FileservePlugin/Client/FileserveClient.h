#pragma once

#include <FileservePlugin/FileservePluginDLL.h>

#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

namespace plDataDirectory
{
  class FileserveType;
}

/// \brief Singleton that represents the client side part of a fileserve connection
///
/// Whether the fileserve plugin will be enabled is controled by plFileserveClient::s_bEnableFileserve
/// By default this is on, but if switched off, the fileserve client functionality will be disabled.
/// plFileserveClient will also switch its functionality off, if the command line argument "-fs_off" is specified.
/// If a program knows that it always wants to switch file serving off, it should either simply not load the plugin at all,
/// or it can inject that command line argument through plCommandLineUtils. This should be done before application startup
/// and especially before any data directories get mounted.
///
/// The timeout for connecting to the server can be configured through the command line option "-fs_timeout seconds"
/// The server to connect to can be configured through command line option "-fs_server address".
/// The default address is "localhost:1042".
class PL_FILESERVEPLUGIN_DLL plFileserveClient
{
  PL_DECLARE_SINGLETON(plFileserveClient);

public:
  plFileserveClient();
  ~plFileserveClient();

  /// \brief Can be called at startup to go through multiple sources and search for a valid server address
  ///
  /// Ie. checks the command line, plFileserve.txt in different directories, etc.
  /// For every potential IP it checks whether a fileserve connection could be established (e.g. tries to connect and
  /// checks whether the server answers). If a valid connection is found, the IP is stored internally and PL_SUCCESS is returned.
  /// Call GetServerConnectionAddress() to retrieve the address.
  ///
  /// \param timeout Specifies the timeout for checking whether a server can be reached.
  plResult SearchForServerAddress(plTime timeout = plTime::MakeFromSeconds(5));

  /// \brief Waits for a Fileserver application to try to connect to this device and send its own information.
  ///
  /// This can be used when a device has no proper way to know the IP through which to connect to a Fileserver.
  /// Instead the device opens a server connection itself, and waits for the other side to try to connect to it.
  /// This typically means that a human has to manually input this device's IP on the host PC into the Fileserve application,
  /// thus enabling the exchange of connection information.
  /// Once this has happened, this function stores the valid server IP internally and returns with success.
  /// A subsequent call to EnsureConnected() should then succeed.
  plResult WaitForServerInfo(plTime timeout = plTime::MakeFromSeconds(60.0 * 5));

  /// \brief Stores the current connection info to a text file in the user data folder.
  plResult SaveCurrentConnectionInfoToDisk() const;

  /// \brief Allows to disable the file serving functionality. Should be called before mounting data directories.
  ///
  /// Also achieved through the command line argument "-fs_off"
  static void DisabledFileserveClient() { s_bEnableFileserve = false; }

  /// \brief Returns the address through which the Fileserve client tried to connect with the server last.
  const char* GetServerConnectionAddress() { return m_sServerConnectionAddress; }

  /// \brief Can be called to ensure a fileserve connection. Otherwise automatically called when a data directory is mounted.
  ///
  /// The timeout defines how long the code will wait for a connection.
  /// Positive numbers are a regular timeout.
  /// A zero timeout means the application will wait indefinitely.
  /// A negative number means to either wait that time, or whatever was specified through the command-line.
  /// The timeout can be specified with the command line switch "-fs_timeout X" (in seconds).
  plResult EnsureConnected(plTime timeout = plTime::MakeFromSeconds(-5));

  /// \brief Needs to be called regularly to update the network. By default this is automatically called when the global event
  /// 'GameApp_UpdatePlugins' is fired, which is done by plGameApplication.
  void UpdateClient();

  /// \brief Adds an address that should be tried for connecting with the server.
  void AddServerAddressToTry(plStringView sAddress);

private:
  friend class plDataDirectory::FileserveType;

  /// \brief True by default, can
  static bool s_bEnableFileserve;

  struct FileCacheStatus
  {
    plInt64 m_TimeStamp = 0;
    plUInt64 m_FileHash = 0;
    plTime m_LastCheck;
  };

  struct DataDir
  {
    // plString m_sRootName;
    // plString m_sPathOnClient;
    plString m_sMountPoint;
    bool m_bMounted = false;

    plMap<plString, FileCacheStatus> m_CacheStatus;
  };

  void DeleteFile(plUInt16 uiDataDir, plStringView sFile);
  plUInt16 MountDataDirectory(plStringView sDataDir, plStringView sRootName);
  void UnmountDataDirectory(plUInt16 uiDataDir);
  static void ComputeDataDirMountPoint(plStringView sDataDir, plStringBuilder& out_sMountPoint);
  void BuildPathInCache(const char* szFile, const char* szMountPoint, plStringBuilder* out_pAbsPath, plStringBuilder* out_pFullPathMeta) const;
  void GetFullDataDirCachePath(const char* szDataDir, plStringBuilder& out_sFullPath, plStringBuilder& out_sFullPathMeta) const;
  void NetworkMsgHandler(plRemoteMessage& msg);
  void HandleFileTransferMsg(plRemoteMessage& msg);
  void HandleFileTransferFinishedMsg(plRemoteMessage& msg);
  static void WriteMetaFile(plStringBuilder sCachedMetaFile, plInt64 iFileTimeStamp, plUInt64 uiFileHash);
  void WriteDownloadToDisk(plStringBuilder sCachedFile);
  plResult DownloadFile(plUInt16 uiDataDirID, const char* szFile, bool bForceThisDataDir, plStringBuilder* out_pFullPath);
  void DetermineCacheStatus(plUInt16 uiDataDirID, const char* szFile, FileCacheStatus& out_Status) const;
  void UploadFile(plUInt16 uiDataDirID, const char* szFile, const plDynamicArray<plUInt8>& fileContent);
  void InvalidateFileCache(plUInt16 uiDataDirID, plStringView sFile, plUInt64 uiHash);
  static plResult TryReadFileserveConfig(const char* szFile, plStringBuilder& out_Result);
  plResult TryConnectWithFileserver(const char* szAddress, plTime timeout) const;
  void FillFileStatusCache(const char* szFile);
  void ShutdownConnection();
  void ClearState();

  mutable plMutex m_Mutex;
  mutable plString m_sServerConnectionAddress;
  plString m_sFileserveCacheFolder;
  plString m_sFileserveCacheMetaFolder;
  bool m_bDownloading = false;
  bool m_bFailedToConnect = false;
  bool m_bWaitingForUploadFinished = false;
  plUuid m_CurFileRequestGuid;
  plStringBuilder m_sCurFileRequest;
  plUniquePtr<plRemoteInterface> m_pNetwork;
  plDynamicArray<plUInt8> m_Download;
  plTime m_CurrentTime;
  plHybridArray<plString, 4> m_TryServerAddresses;

  plMap<plString, plUInt16> m_FileDataDir;
  plHybridArray<DataDir, 8> m_MountedDataDirs;
};
