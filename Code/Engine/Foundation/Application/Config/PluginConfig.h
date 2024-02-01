#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/String.h>

class PL_FOUNDATION_DLL plApplicationPluginConfig
{
public:
  plApplicationPluginConfig();

  static constexpr const plStringView s_sConfigFile = ":project/RuntimeConfigs/Plugins.ddl"_plsv;

  plResult Save(plStringView sConfigPath = s_sConfigFile) const;
  void Load(plStringView sConfigPath = s_sConfigFile);
  void Apply();

  struct PL_FOUNDATION_DLL PluginConfig
  {
    bool operator<(const PluginConfig& rhs) const;

    plString m_sAppDirRelativePath;
    bool m_bLoadCopy = false;
  };

  bool AddPlugin(const PluginConfig& cfg);
  bool RemovePlugin(const PluginConfig& cfg);

  mutable plHybridArray<PluginConfig, 8> m_Plugins;
};


using plApplicationPluginConfig_PluginConfig = plApplicationPluginConfig::PluginConfig;

PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plApplicationPluginConfig);
PL_DECLARE_REFLECTABLE_TYPE(PL_FOUNDATION_DLL, plApplicationPluginConfig_PluginConfig);
