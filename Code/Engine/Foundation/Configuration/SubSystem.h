#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Utilities/EnumerableClass.h>

/// \brief Describes the different stages during startup and shutdown
struct plStartupStage
{
  enum Enum
  {
    None = -1,
    BaseSystems,      ///< In this stage the absolute base functionality is started. This should only be used by the Foundation library.
    CoreSystems,      ///< In this stage the core functionality is being started / shut down
    HighLevelSystems, ///< In this stage the higher level functionality, which depends on a rendering context, is being started / shut down

    ENUM_COUNT
  };
};

/// \brief Base class for all subsystems.
///
/// plStartup will initialize and shut down all instances of this class, according to their dependencies etc.
/// If you have a subsystem that is a non-static class, just derive from this base class and override the
/// virtual functions as required.
/// If you have a subsystem that is implemented in a purely static way (there is no class instance),
/// just use the macros PL_BEGIN_SUBSYSTEM_DECLARATION, PL_END_SUBSYSTEM_DECLARATION etc.
/// Those macros will create a wrapper object (derived from plSubSystem) to handle initialization.
class PL_FOUNDATION_DLL plSubSystem : public plEnumerable<plSubSystem>
{
  PL_DECLARE_ENUMERABLE_CLASS(plSubSystem);
  PL_DISALLOW_COPY_AND_ASSIGN(plSubSystem);

public:
  plSubSystem()
  {
    for (plInt32 i = 0; i < plStartupStage::ENUM_COUNT; ++i)
      m_bStartupDone[i] = false;
  }

  virtual ~plSubSystem() = default;

  /// \brief Returns the name of the subsystem. Must be overridden.
  virtual plStringView GetSubSystemName() const = 0;

  /// \brief Returns the name of the group to which this subsystem belongs. Must be overridden.
  virtual plStringView GetGroupName() const = 0;

  /// \brief Returns a series of strings with the names of the subsystem, which this subsystem depends on. nullptr indicates the last entry.
  /// Must be overridden.
  virtual plStringView GetDependency(plInt32 iDep) { return {}; }

  /// \brief Returns the plugin name to which this subsystem belongs.
  plStringView GetPluginName() const { return m_sPluginName; }

  /// \brief Returns whether the given startup stage has been done on this subsystem.
  bool IsStartupPhaseDone(plStartupStage::Enum stage) const { return m_bStartupDone[stage]; }

private:
  // only the startup system may access the following functionality
  friend class plStartup;

  /// \brief This will be called to initialize the subsystems base components. Can be overridden to handle this event.
  virtual void OnBaseSystemsStartup() {}

  /// \brief This will be called to initialize the subsystems core components. Can be overridden to handle this event.
  virtual void OnCoreSystemsStartup() {}

  /// \brief This will be called to shut down the subsystems core components. Can be overridden to handle this event.
  virtual void OnCoreSystemsShutdown() {}

  /// \brief This will be called to initialize the subsystems engine / rendering components. Can be overridden to handle this event.
  virtual void OnHighLevelSystemsStartup() {}

  /// \brief This will be called to shut down the subsystems engine / rendering components. Can be overridden to handle this event.
  virtual void OnHighLevelSystemsShutdown() {}

  /// Set by plStartup to store to which plugin this subsystem belongs.
  plStringView m_sPluginName;

  /// Stores which startup phase has been done already.
  bool m_bStartupDone[plStartupStage::ENUM_COUNT];
};

#include <Foundation/Configuration/StaticSubSystem.h>
