#include <Foundation/Application/Application.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Utilities/CommandLineUtils.h>

/* When statically linking libraries into an application the linker will only pull in all the functions and variables that are inside
translation units (CPP files) that somehow get referenced.

In pl a lot of stuff happens automatically (e.g. types register themselves etc.), which is accomplished through global variables
that execute code in their constructor during the applications startup phase. This only works when those global variables are actually
put into the application by the linker. If the linker does not do that, functionality will not work as intended.

Contrary to common conception, the linker is NOT ALLOWED to optimize away global variables. The only reason for not including a global
variable into the final binary, is when the entire translation unit where a variable is defined in, is never referenced and thus never
even looked at by the linker.

To fix this, this tool inserts macros into each and every file which reference each other. Afterwards every file in a library will
have reference every other file in that same library and thus once a library is used in any way in some program, the entire library
will be pulled in and will then work as intended.

These references are accomplished through empty functions that are called in one central location (where PL_STATICLINK_LIBRARY is defined),
though the code actually never really calls those functions, but it is enough to force the linker to look at all the other files.

Usage of this tool:

Call this tool with the path to the root folder of some library as the sole command line argument:

StaticLinkUtil.exe "C:\plEngine\Trunk\Code\Engine\Foundation"

This will iterate over all files below that folder and insert the proper macros.
Also make sure that exactly one file in each library contains the text 'PL_STATICLINK_LIBRARY();'

The parameters and function body will be automatically generated and later updated, you do not need to provide more.

See the Return Codes at the end of the BeforeCoreSystemsShutdown function.
*/

class plStaticLinkerApp : public plApplication
{
private:
  plString m_sSearchDir;
  bool m_bHadErrors;
  bool m_bHadSeriousWarnings;
  bool m_bHadWarnings;
  bool m_bModifiedFiles;
  bool m_bAnyFileChanged;

  struct FileContent
  {
    FileContent() { m_bFileHasChanged = false; }

    bool m_bFileHasChanged;
    plString m_sFileContent;
  };

  plSet<plString> m_AllRefPoints;
  plString m_sRefPointGroupFile;

  plSet<plString> m_GlobalIncludes;

  plMap<plString, FileContent> m_ModifiedFiles;

  plSet<plString> m_FilesToModify;
  plSet<plString> m_FilesToLink;


public:
  using SUPER = plApplication;

  plStaticLinkerApp()
    : plApplication("StaticLinkerApp")
  {
    m_bHadErrors = false;
    m_bHadSeriousWarnings = false;
    m_bHadWarnings = false;
    m_bModifiedFiles = false;
    m_bAnyFileChanged = false;
  }

  /// Makes sure the apps return value reflects whether there were any errors or warnings
  static void LogInspector(const plLoggingEventData& eventData)
  {
    plStaticLinkerApp* app = (plStaticLinkerApp*)plApplication::GetApplicationInstance();

    switch (eventData.m_EventType)
    {
      case plLogMsgType::ErrorMsg:
        app->m_bHadErrors = true;
        break;
      case plLogMsgType::SeriousWarningMsg:
        app->m_bHadSeriousWarnings = true;
        break;
      case plLogMsgType::WarningMsg:
        app->m_bHadWarnings = true;
        break;

      default:
        break;
    }
  }

  virtual void AfterCoreSystemsStartup() override
  {
    plGlobalLog::AddLogWriter(plLogWriter::Console::LogMessageHandler);
    plGlobalLog::AddLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
    plGlobalLog::AddLogWriter(LogInspector);

    if (GetArgumentCount() != 2)
      plLog::Error("This tool requires exactly one command-line argument: A path to the top-level folder of a library.");

    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    plStringBuilder sSearchDir = plOSFile::MakePathAbsoluteWithCWD(GetArgument(1));

    m_sSearchDir = sSearchDir;

    // Add the empty data directory to access files via absolute paths
    plFileSystem::AddDataDirectory("", "App", ":", plFileSystem::AllowWrites).IgnoreResult();

    // use such a path to write to an absolute file
    // ':abs/C:/some/file.txt"
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    if ((m_bHadSeriousWarnings || m_bHadErrors) && m_bModifiedFiles)
      plLog::SeriousWarning("There were issues while writing out the updated files. The source will be in an inconsistent state, please revert the changes.");
    else if (m_bHadWarnings || m_bHadSeriousWarnings || m_bHadErrors)
    {
      plLog::Warning("There have been errors or warnings, see log for details.");
    }

    if (m_bModifiedFiles)
    {
      if (m_bHadErrors || m_bHadSeriousWarnings)
        SetReturnCode(3); // Errors or Serious Warnings, yet files modified, this is unusual, requires reverting the source from outside.
      else if (m_bHadWarnings)
        SetReturnCode(2); // Warnings, files still modified. (normal operation)
      else
        SetReturnCode(1); // No issues, files modified. (normal operation)
    }
    else
    {
      if (m_bHadErrors || m_bHadSeriousWarnings)
        SetReturnCode(-3); // Errors or serious warnings, no files modified (but might need to be), user needs to look at it.
      else if (m_bHadWarnings)
        SetReturnCode(-2); // Warnings, but no files were modified anyway, user should look at it though. (normal operation)
      else
        SetReturnCode(-1); // No issues, no file modifications, everything is up to date apparently. (normal operation)
    }

    // Return Codes:
    // All negative requires no RCS operations (no changes were made)
    // All positive require RCS operations (either commit or revert)
    // -1 and 1 are perfectly fine
    // -2 and 2 mean there were warnings that a user should look at, but nothing that prevented this tool from doing its work
    // -3 and 3 mean something went wrong
    // thus 3 means the changes it made need to be reverted from outside
    // 1 and 2 mean the changes need to be committed


    plGlobalLog::RemoveLogWriter(LogInspector);
    plGlobalLog::RemoveLogWriter(plLogWriter::Console::LogMessageHandler);
    plGlobalLog::RemoveLogWriter(plLogWriter::VisualStudio::LogMessageHandler);
  }

  plString GetLibraryMarkerName()
  {
    plStringBuilder tmp;
    return plPathUtils::GetFileName(m_sSearchDir.GetData()).GetData(tmp);
  }

  void SanitizeSourceCode(plStringBuilder& ref_sInOut)
  {
    // this is now handled by clang-format and .editorconfig files

    // ref_sInOut.ReplaceAll("\r\n", "\n");
    // if (!ref_sInOut.EndsWith("\n"))
    //  ref_sInOut.Append("\n");

    while (ref_sInOut.EndsWith("\n\n\n\n"))
      ref_sInOut.Shrink(0, 1);
    while (ref_sInOut.EndsWith("\r\n\r\n\r\n\r\n"))
      ref_sInOut.Shrink(0, 2);
  }

  plResult ReadEntireFile(plStringView sFile, plStringBuilder& ref_sOut)
  {
    ref_sOut.Clear();

    // if we have that file cached already, just return the cached (and possibly modified) content
    if (!m_ModifiedFiles[sFile].m_sFileContent.IsEmpty())
    {
      ref_sOut = m_ModifiedFiles[sFile].m_sFileContent.GetData();
      return PL_SUCCESS;
    }

    plFileReader File;
    if (File.Open(sFile) == PL_FAILURE)
    {
      plLog::Error("Could not open for reading: '{0}'", sFile);
      return PL_FAILURE;
    }

    plDynamicArray<plUInt8> FileContent;

    plUInt8 Temp[1024];
    plUInt64 uiRead = File.ReadBytes(Temp, 1024);

    while (uiRead > 0)
    {
      FileContent.PushBackRange(plArrayPtr<plUInt8>(Temp, (plUInt32)uiRead));

      uiRead = File.ReadBytes(Temp, 1024);
    }

    FileContent.PushBack(0);

    if (!plUnicodeUtils::IsValidUtf8((const char*)&FileContent[0]))
    {
      plLog::Error("The file \"{0}\" contains characters that are not valid Utf8. This often happens when you type special characters in an editor "
                   "that does not save the file in Utf8 encoding.",
        sFile);
      return PL_FAILURE;
    }

    ref_sOut = (const char*)&FileContent[0];

    m_ModifiedFiles[sFile].m_sFileContent = ref_sOut;

    SanitizeSourceCode(ref_sOut);

    return PL_SUCCESS;
  }

  void OverwriteFile(plStringView sFile, const plStringBuilder& sFileContent)
  {
    plStringBuilder sOut = sFileContent;
    SanitizeSourceCode(sOut);

    if (m_ModifiedFiles[sFile].m_sFileContent == sOut)
      return;

    m_bAnyFileChanged = true;
    m_ModifiedFiles[sFile].m_bFileHasChanged = true;
    m_ModifiedFiles[sFile].m_sFileContent = sOut;
  }

  void OverwriteModifiedFiles()
  {
    PL_LOG_BLOCK("Overwriting modified files");

    if (m_bHadSeriousWarnings || m_bHadErrors)
    {
      plLog::Info("There have been errors or warnings previously, no files will be modified.");
      return;
    }

    if (!m_bAnyFileChanged)
    {
      plLog::Success("No files needed modification.");
      return;
    }

    for (auto it = m_ModifiedFiles.GetIterator(); it.IsValid(); ++it)
    {
      if (!it.Value().m_bFileHasChanged)
        continue;

      plFileWriter FileOut;
      if (FileOut.Open(it.Key().GetData()) == PL_FAILURE)
      {
        plLog::Error("Could not open the file for writing: '{0}'", it.Key());
        return;
      }
      else
      {
        m_bModifiedFiles = true;
        FileOut.WriteBytes(it.Value().m_sFileContent.GetData(), it.Value().m_sFileContent.GetElementCount()).IgnoreResult();

        plLog::Success("File has been modified: '{0}'", it.Key());
      }
    }
  }

  void FindIncludes(plStringBuilder& ref_sFileContent)
  {
    const char* szStartPos = ref_sFileContent.GetData();
    const plString sLibraryName = GetLibraryMarkerName();

    while (true)
    {
      const char* szI = ref_sFileContent.FindSubString("#i", szStartPos);

      if (szI == nullptr)
        return;

      szStartPos = szI + 1;

      if (plStringUtils::IsEqualN(szI, "#if", 3))
      {
        szStartPos = ref_sFileContent.FindSubString("#endif", szStartPos);

        if (szStartPos == nullptr)
          return;

        ++szStartPos;
        continue; // next search will be for #i again
      }

      if (plStringUtils::IsEqualN(szI, "#include", 8))
      {
        szI += 8; // skip the "#include" string

        const char* szLineEnd = plStringUtils::FindSubString(szI, "\n");

        plStringView si(szI, szLineEnd);

        plStringBuilder sInclude = si;

        if (sInclude.ReplaceAll("\\", "/") > 0)
        {
          plLog::Info("Replacing backslashes in #include path with front slashes: '{0}'", sInclude);
          ref_sFileContent.ReplaceSubString(szI, szLineEnd, sInclude.GetData());
        }

        while (sInclude.StartsWith(" ") || sInclude.StartsWith("\t") || sInclude.StartsWith("<"))
          sInclude.Shrink(1, 0);

        while (sInclude.EndsWith(" ") || sInclude.EndsWith("\t") || sInclude.EndsWith(">"))
          sInclude.Shrink(0, 1);

        // ignore relative includes, they will not work as expected from the PCH
        if (sInclude.StartsWith("\""))
          continue;

        // ignore includes into the own library
        if (sInclude.StartsWith(sLibraryName.GetData()))
          continue;

        // ignore third-party includes
        if (sInclude.FindSubString_NoCase("ThirdParty"))
        {
          plLog::Dev("Skipping ThirdParty Include: '{0}'", sInclude);
          continue;
        }

        plStringBuilder sCanFindInclude = m_sSearchDir.GetData();
        sCanFindInclude.PathParentDirectory();
        sCanFindInclude.AppendPath(sInclude.GetData());

        plStringBuilder sCanFindInclude2 = m_sSearchDir.GetData();
        sCanFindInclude2.PathParentDirectory(2);
        sCanFindInclude2.AppendPath("Code/Engine");
        sCanFindInclude2.AppendPath(sInclude.GetData());

        // ignore includes to files that cannot be found (ie. they are not part of the plEngine source tree)
        if (!plFileSystem::ExistsFile(sCanFindInclude.GetData()) && !plFileSystem::ExistsFile(sCanFindInclude2.GetData()))
        {
          plLog::Dev("Skipping non-Engine Include: '{0}'", sInclude);
          continue;
        }

        // warn about includes that have 'implementation' in their path
        if (sInclude.FindSubString_NoCase("Implementation"))
        {
          plLog::Warning("This file includes an implementation header from another library: '{0}'", sInclude);
        }

        plLog::Dev("Found Include: '{0}'", sInclude);

        m_GlobalIncludes.Insert(sInclude);
      }
    }
  }

  bool RemoveLineWithPrefix(plStringBuilder& ref_sFile, plStringView sLineStart)
  {
    const char* szSkipAhead = ref_sFile.FindSubString("// <StaticLinkUtil::StartHere>");

    const char* szStart = ref_sFile.FindSubString(sLineStart, szSkipAhead);

    if (szStart == nullptr)
      return false;

    const char* szEnd = ref_sFile.FindSubString("\n", szStart);

    if (szEnd == nullptr)
      szEnd = ref_sFile.GetData() + ref_sFile.GetElementCount();

    ref_sFile.ReplaceSubString(szStart, szEnd, "");

    return true;
  }

  void RewritePrecompiledHeaderIncludes()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    plStringBuilder sPCHFile = m_sSearchDir.GetData();
    sPCHFile.AppendPath("PCH.h");

    {
      plFileReader File;
      if (File.Open(sPCHFile.GetData()) == PL_FAILURE)
      {
        plLog::Warning("This project has no PCH file.");
        return;
      }
    }

    plLog::Info("Rewriting PCH: '{0}'", sPCHFile);

    plStringBuilder sFileContent;
    if (ReadEntireFile(sPCHFile.GetData(), sFileContent) == PL_FAILURE)
      return;

    while (RemoveLineWithPrefix(sFileContent, "#include"))
    {
      // do this
    }

    SanitizeSourceCode(sFileContent);

    plStringBuilder sAllIncludes;

    for (auto it = m_GlobalIncludes.GetIterator(); it.IsValid(); ++it)
    {
      sAllIncludes.AppendFormat("#include <{0}>\n", it.Key());
    }

    sAllIncludes.ReplaceAll("\\", "/");

    sFileContent.Append(sAllIncludes.GetData());

    OverwriteFile(sPCHFile.GetData(), sFileContent);
  }

  void FixFileContents(plStringView sFile)
  {
    plStringBuilder sFileContent;
    if (ReadEntireFile(sFile, sFileContent) == PL_FAILURE)
      return;

    if (sFile.EndsWith("PCH.h"))
      plLog::Dev("Skipping PCH for #include search: '{0}'", sFile);
    else
      FindIncludes(sFileContent);

    // rewrite the entire file
    OverwriteFile(sFile, sFileContent);
  }

  plString GetFileMarkerName(plStringView sFile)
  {
    plStringBuilder sRel = sFile;
    sRel.MakeRelativeTo(m_sSearchDir.GetData()).IgnoreResult();

    plStringBuilder sRefPointName = plPathUtils::GetFileName(m_sSearchDir.GetData());
    sRefPointName.Append("_");
    sRefPointName.Append(sRel.GetData());
    sRefPointName.ReplaceAll("\\", "_");
    sRefPointName.ReplaceAll("/", "_");
    sRefPointName.ReplaceAll(".cpp", "");

    return sRefPointName;
  }

  void InsertRefPoint(plStringView sFile)
  {
    PL_LOG_BLOCK("InsertRefPoint", sFile);

    plStringBuilder sFileContent;
    if (ReadEntireFile(sFile, sFileContent) == PL_FAILURE)
      return;

    // if we find this macro in here, we don't need to insert PL_STATICLINK_FILE in this file
    // but once we are done with all files, we want to come back to this file and rewrite the PL_STATICLINK_LIBRARY
    // part such that it will reference all the other files
    if (sFileContent.FindSubString("PL_STATICLINK_LIBRARY"))
      return;

    plString sLibraryMarker = GetLibraryMarkerName();
    plString sFileMarker = GetFileMarkerName(sFile);

    plStringBuilder sNewMarker;

    if (m_FilesToLink.Contains(sFile))
    {
      m_AllRefPoints.Insert(sFileMarker.GetData());
      sNewMarker.SetFormat("PL_STATICLINK_FILE({0}, {1});", sLibraryMarker, sFileMarker);
    }

    const char* szMarker = sFileContent.FindSubString("PL_STATICLINK_FILE");

    // if the marker already exists, replace it with the updated string
    if (szMarker != nullptr)
    {
      const char* szMarkerEnd = szMarker;

      while (*szMarkerEnd != '\0' && *szMarkerEnd != '\n')
        ++szMarkerEnd;

      sFileContent.ReplaceSubString(szMarker, szMarkerEnd, sNewMarker.GetData());
    }
    else
    {
      // otherwise insert it at the end of the file
      sFileContent.AppendFormat("\n\n{0}\n\n", sNewMarker);
    }

    // rewrite the entire file
    OverwriteFile(sFile, sFileContent);
  }

  void UpdateStaticLinkLibraryBlock()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    plStringView sFile = m_sRefPointGroupFile;

    PL_LOG_BLOCK("RewriteRefPointGroup", sFile);

    plLog::Info("Replacing macro PL_STATICLINK_LIBRARY in file '{0}'.", m_sRefPointGroupFile);

    plStringBuilder sFileContent;
    if (ReadEntireFile(sFile, sFileContent) == PL_FAILURE)
      return;

    // remove all instances of PL_STATICLINK_FILE from this file, it already contains PL_STATICLINK_LIBRARY
    const char* szMarker = sFileContent.FindSubString("PL_STATICLINK_FILE");
    while (szMarker != nullptr)
    {
      plLog::Warning("Found macro PL_STATICLINK_FILE inside the same file where PL_STATICLINK_LIBRARY is located. Removing it.");

      const char* szMarkerEnd = szMarker;

      while (*szMarkerEnd != '\0' && *szMarkerEnd != '\n')
        ++szMarkerEnd;

      // no ref point allowed in a file that has already a ref point group
      sFileContent.Remove(szMarker, szMarkerEnd);

      szMarker = sFileContent.FindSubString("PL_STATICLINK_FILE");
    }

    plStringBuilder sNewGroupMarker;

    // generate the code that should be inserted into this file
    // this code will reference all the other files in the library
    {
      sNewGroupMarker.SetFormat("PL_STATICLINK_LIBRARY({0})\n{\n  if (bReturn)\n    return;\n\n", GetLibraryMarkerName());

      auto it = m_AllRefPoints.GetIterator();

      while (it.IsValid())
      {
        sNewGroupMarker.AppendFormat("  PL_STATICLINK_REFERENCE({0});\n", it.Key());
        ++it;
      }

      sNewGroupMarker.Append("}\n");
    }

    const char* szGroupMarker = sFileContent.FindSubString("PL_STATICLINK_LIBRARY");

    if (szGroupMarker != nullptr)
    {
      // if we could find the macro PL_STATICLINK_LIBRARY, just replace it with the new code

      const char* szMarkerEnd = szGroupMarker;

      bool bFoundOpenBraces = false;

      while (*szMarkerEnd != '\0' && *szMarkerEnd != '}')
      {
        ++szMarkerEnd;

        if (*szMarkerEnd == '{')
          bFoundOpenBraces = true;
        if (!bFoundOpenBraces && *szMarkerEnd == ';')
          break;
      }

      if (*szMarkerEnd == '}' || *szMarkerEnd == ';')
        ++szMarkerEnd;
      if (*szMarkerEnd == '\n')
        ++szMarkerEnd;

      // now replace the existing PL_STATICLINK_LIBRARY and its code block with the new block
      sFileContent.ReplaceSubString(szGroupMarker, szMarkerEnd, sNewGroupMarker.GetData());
    }
    else
    {
      // if we can't find the macro, append it to the end of the file
      // this can only happen, if we ever extend this tool such that it picks one file to auto-insert this macro
      sFileContent.AppendFormat("\n\n{0}\n\n", sNewGroupMarker);
    }

    OverwriteFile(sFile, sFileContent);
  }

  void IterateOverFiles()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    plStringBuilder b, sExt;

    for (const plString& sFile : m_FilesToModify)
    {
      if (sFile.HasExtension("h") || sFile.HasExtension("inl"))
      {
        PL_LOG_BLOCK("Header", sFile.GetFileNameAndExtension().GetStartPointer());
        FixFileContents(sFile);
        continue;
      }

      if (sFile.HasExtension("cpp"))
      {
        PL_LOG_BLOCK("Source", sFile.GetFileNameAndExtension().GetStartPointer());
        FixFileContents(sFile);

        InsertRefPoint(sFile);
        continue;
      }
    }
  }

  void MakeSureStaticLinkLibraryMacroExists()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    // The macro PL_STATICLINK_LIBRARY was not found in any cpp file
    // try to insert it into a PCH.cpp, if there is one
    if (!m_sRefPointGroupFile.IsEmpty())
      return;

    plStringBuilder sFilePath;
    sFilePath.AppendPath(m_sSearchDir.GetData(), "PCH.cpp");

    auto it = m_ModifiedFiles.Find(sFilePath);

    if (it.IsValid())
    {
      plStringBuilder sPCHcpp = it.Value().m_sFileContent.GetData();
      sPCHcpp.Append("\n\n\n\nPL_STATICLINK_LIBRARY() { }");

      OverwriteFile(sFilePath.GetData(), sPCHcpp);

      m_sRefPointGroupFile = sFilePath;

      plLog::Warning("No PL_STATICLINK_LIBRARY found in any cpp file, inserting it into the PCH.cpp file.");
    }
    else
      plLog::Error("The macro PL_STATICLINK_LIBRARY was not found in any cpp file in this library. It is required that it exists in exactly one "
                   "file, otherwise the generated code will not compile.");
  }

  void GatherInformation()
  {
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return;

    PL_LOG_BLOCK("FindRefPointGroupFile");

#if PL_ENABLED(PL_SUPPORTS_FILE_ITERATORS) || defined(PL_DOCS)
    // get a directory iterator for the search directory
    plFileSystemIterator it;
    it.StartSearch(m_sSearchDir.GetData(), plFileSystemIteratorFlags::ReportFilesRecursive);

    if (it.IsValid())
    {
      plStringBuilder sFile, sExt;

      // while there are additional files / folders
      for (; it.IsValid(); it.Next())
      {
        // build the absolute path to the current file
        sFile = it.GetCurrentPath();
        sFile.AppendPath(it.GetStats().m_sName.GetData());

        // file extensions are always converted to lower-case actually
        sExt = sFile.GetFileExtension();

        if (sExt.IsEqual_NoCase("h") || sExt.IsEqual_NoCase("inl"))
        {
          m_FilesToModify.Insert(sFile);
        }

        if (sExt.IsEqual_NoCase("cpp"))
        {
          plStringBuilder sFileContent;
          if (ReadEntireFile(sFile.GetData(), sFileContent) == PL_FAILURE)
            return;

          // if we find this macro in here, we don't need to insert PL_STATICLINK_FILE in this file
          // but once we are done with all files, we want to come back to this file and rewrite the PL_STATICLINK_LIBRARY
          // part such that it will reference all the other files
          if (sFileContent.FindSubString("PL_STATICLINK_LIBRARY"))
          {
            m_FilesToModify.Insert(sFile);

            plLog::Info("Found macro 'PL_STATICLINK_LIBRARY' in file '{0}'.", &sFile.GetData()[m_sSearchDir.GetElementCount() + 1]);

            if (!m_sRefPointGroupFile.IsEmpty())
              plLog::Error("The macro 'PL_STATICLINK_LIBRARY' was already found in file '{0}' before. You cannot have this macro twice in the same library!", m_sRefPointGroupFile);
            else
              m_sRefPointGroupFile = sFile;
          }

          if (sFileContent.FindSubString("PL_STATICLINK_FILE_DISABLE"))
            continue;

          m_FilesToModify.Insert(sFile);

          bool bContainsGlobals = false;

          bContainsGlobals = bContainsGlobals || (sFileContent.FindSubString("PL_STATICLINK_FORCE") != nullptr);
          bContainsGlobals = bContainsGlobals || (sFileContent.FindSubString("PL_STATICLINK_LIBRARY") != nullptr);
          bContainsGlobals = bContainsGlobals || (sFileContent.FindSubString("PL_BEGIN_") != nullptr);
          bContainsGlobals = bContainsGlobals || (sFileContent.FindSubString("PL_PLUGIN_") != nullptr);
          bContainsGlobals = bContainsGlobals || (sFileContent.FindSubString("PL_ON_GLOBAL_EVENT") != nullptr);
          bContainsGlobals = bContainsGlobals || (sFileContent.FindSubString("plCVarBool ") != nullptr);
          bContainsGlobals = bContainsGlobals || (sFileContent.FindSubString("plCVarFloat ") != nullptr);
          bContainsGlobals = bContainsGlobals || (sFileContent.FindSubString("plCVarInt ") != nullptr);
          bContainsGlobals = bContainsGlobals || (sFileContent.FindSubString("plCVarString ") != nullptr);

          if (!bContainsGlobals)
            continue;

          m_FilesToLink.Insert(sFile);
        }
      }
    }
    else
      plLog::Error("Could not search the directory '{0}'", m_sSearchDir);
#else
    PL_REPORT_FAILURE("No file system iterator support, StaticLinkUtil sample can't run.");
#endif
    MakeSureStaticLinkLibraryMacroExists();
  }

  virtual plApplication::Execution Run() override
  {
    // something basic has gone wrong
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return plApplication::Execution::Quit;

    GatherInformation();

    IterateOverFiles();

    UpdateStaticLinkLibraryBlock();

    // RewritePrecompiledHeaderIncludes();

    OverwriteModifiedFiles();

    return plApplication::Execution::Quit;
  }
};

PL_CONSOLEAPP_ENTRY_POINT(plStaticLinkerApp);
