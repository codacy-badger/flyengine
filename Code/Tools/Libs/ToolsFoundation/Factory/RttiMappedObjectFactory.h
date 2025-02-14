#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// \brief A factory that creates the closest matching objects according to the passed type.
///
/// Creators can be registered at the factory for a specific type.
/// When the create function is called for a type, the parent type hierarchy is traversed until
/// the first type is found for which a creator is registered.
template <typename Object>
class plRttiMappedObjectFactory
{
  PL_DISALLOW_COPY_AND_ASSIGN(plRttiMappedObjectFactory);

public:
  plRttiMappedObjectFactory();
  ~plRttiMappedObjectFactory();

  using CreateObjectFunc = Object* (*)(const plRTTI*);

  void RegisterCreator(const plRTTI* pType, CreateObjectFunc creator);
  void UnregisterCreator(const plRTTI* pType);
  Object* CreateObject(const plRTTI* pType);

  struct Event
  {
    enum class Type
    {
      CreatorAdded,
      CreatorRemoved
    };

    Type m_Type;
    const plRTTI* m_pRttiType;
  };

  plEvent<const Event&> m_Events;

private:
  plHashTable<const plRTTI*, CreateObjectFunc> m_Creators;
};

#include <ToolsFoundation/Factory/Implementation/RttiMappedObjectFactory_inl.h>
