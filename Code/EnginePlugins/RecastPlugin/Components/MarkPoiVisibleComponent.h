#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>
#include <RecastPlugin/RecastPluginDLL.h>

class plRecastWorldModule;
class plPhysicsWorldModuleInterface;

//////////////////////////////////////////////////////////////////////////

using plRcMarkPoiVisibleComponentManager = plComponentManagerSimple<class plRcMarkPoiVisibleComponent, plComponentUpdateType::WhenSimulating>;

class PL_RECASTPLUGIN_DLL plRcMarkPoiVisibleComponent : public plRcComponent
{
  PL_DECLARE_COMPONENT_TYPE(plRcMarkPoiVisibleComponent, plRcComponent, plRcMarkPoiVisibleComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // plRcMarkPoiVisibleComponent

public:
  plRcMarkPoiVisibleComponent();
  ~plRcMarkPoiVisibleComponent();

  float m_fRadius = 20.0f;        // [ property ]
  plUInt8 m_uiCollisionLayer = 0; // [ property ]

protected:
  void Update();

  plRecastWorldModule* m_pWorldModule = nullptr;
  plPhysicsWorldModuleInterface* m_pPhysicsModule = nullptr;

private:
  plUInt32 m_uiLastFirstCheckedPoint = 0;
};
