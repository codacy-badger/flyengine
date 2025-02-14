#pragma once

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/World/World.h>
#include <RendererCore/RendererCoreDLL.h>

/// \brief Base class for objects that should be rendered.
class PL_RENDERERCORE_DLL plRenderComponent : public plComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plRenderComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

protected:
  virtual void Deinitialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plRenderComponent

public:
  plRenderComponent();
  ~plRenderComponent();

  /// \brief Called by plRenderComponent::OnUpdateLocalBounds().
  /// If PL_SUCCESS is returned, \a bounds and \a bAlwaysVisible will be integrated into the plMsgUpdateLocalBounds result,
  /// otherwise the out values are simply ignored.
  virtual plResult GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg) = 0;

  /// \brief Call this when some value was modified that affects the size of the local bounding box and it should be recomputed.
  void TriggerLocalBoundsUpdate();

  /// \brief Computes a unique ID for the given component, that is usually given to the renderer to distinguish objects.
  static plUInt32 GetUniqueIdForRendering(const plComponent& component, plUInt32 uiInnerIndex = 0, plUInt32 uiInnerIndexShift = 24);

  /// \brief Computes a unique ID for the given component, that is usually given to the renderer to distinguish objects.
  PL_ALWAYS_INLINE plUInt32 GetUniqueIdForRendering(plUInt32 uiInnerIndex = 0, plUInt32 uiInnerIndexShift = 24) const
  {
    return GetUniqueIdForRendering(*this, uiInnerIndex, uiInnerIndexShift);
  }

protected:
  void OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg);
  void InvalidateCachedRenderData();
};
