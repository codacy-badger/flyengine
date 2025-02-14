#pragma once

#include <Core/Graphics/Camera.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/XRInputDevice.h>
#include <GameEngine/XR/XRInterface.h>
#include <OpenXRPlugin/Basics.h>
#include <OpenXRPlugin/OpenXRIncludes.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/Shader/ConstantBufferStorage.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/SwapChain.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

class plOpenXRInputDevice;
class plOpenXRSpatialAnchors;
class plOpenXRHandTracking;
class plWindowOutputTargetXR;
struct plGameApplicationExecutionEvent;

PL_DEFINE_AS_POD_TYPE(XrViewConfigurationView);
PL_DEFINE_AS_POD_TYPE(XrEnvironmentBlendMode);

class PL_OPENXRPLUGIN_DLL plOpenXR : public plXRInterface
{
  PL_DECLARE_SINGLETON_OF_INTERFACE(plOpenXR, plXRInterface);

public:
  plOpenXR();
  ~plOpenXR();

  XrInstance GetInstance() const { return m_pInstance; }
  uint64_t GetSystemId() const { return m_SystemId; }
  XrSession GetSession() const { return m_pSession; }
  XrViewConfigurationType GetViewType() const { return m_PrimaryViewConfigurationType; }
  bool GetDepthComposition() const;

  virtual bool IsHmdPresent() const override;

  virtual plResult Initialize() override;
  virtual void Deinitialize() override;
  virtual bool IsInitialized() const override;

  virtual const plHMDInfo& GetHmdInfo() const override;
  virtual plXRInputDevice& GetXRInput() const override;

  virtual plGALTextureHandle GetCurrentTexture() override;

  void DelayPresent();
  void Present();
  void EndFrame();

  virtual plUniquePtr<plActor> CreateActor(plView* pView, plGALMSAASampleCount::Enum msaaCount = plGALMSAASampleCount::None,
    plUniquePtr<plWindowBase> pCompanionWindow = nullptr, plUniquePtr<plWindowOutputTargetGAL> pCompanionWindowOutput = nullptr) override;
  virtual void OnActorDestroyed() override;
  virtual bool SupportsCompanionView() override;

  XrSpace GetBaseSpace() const;

private:
  XrResult SelectExtensions(plHybridArray<const char*, 6>& extensions);
  XrResult SelectLayers(plHybridArray<const char*, 6>& layers);
  XrResult InitSystem();
  void DeinitSystem();
  XrResult InitSession();
  void DeinitSession();
  XrResult InitGraphicsPlugin();
  void DeinitGraphicsPlugin();
  XrResult InitDebugMessenger();
  void DeinitInitDebugMessenger();

  void GameApplicationEventHandler(const plGameApplicationExecutionEvent& e);
  void GALDeviceEventHandler(const plGALDeviceEvent& e);

  void BeforeUpdatePlugins();
  void UpdatePoses();
  void UpdateCamera();
  void BeginFrame();

  void SetStageSpace(plXRStageSpace::Enum space);
  void SetHMDCamera(plCamera* pCamera);

  plWorld* GetWorld();

public:
  static XrPosef ConvertTransform(const plTransform& tr);
  static XrQuaternionf ConvertOrientation(const plQuat& q);
  static XrVector3f ConvertPosition(const plVec3& vPos);
  static plQuat ConvertOrientation(const XrQuaternionf& q);
  static plVec3 ConvertPosition(const XrVector3f& pos);
  static plMat4 ConvertPoseToMatrix(const XrPosef& pose);
  static plGALResourceFormat::Enum ConvertTextureFormat(int64_t format);

private:
  friend class plOpenXRInputDevice;
  friend class plOpenXRSpatialAnchors;
  friend class plOpenXRHandTracking;
  friend class plOpenXRRemoting;
  friend class plGALOpenXRSwapChain;

  struct Extensions
  {
    bool m_bValidation = false;
    bool m_bDebugUtils = false;
    PFN_xrCreateDebugUtilsMessengerEXT pfn_xrCreateDebugUtilsMessengerEXT;
    PFN_xrDestroyDebugUtilsMessengerEXT pfn_xrDestroyDebugUtilsMessengerEXT;

    bool m_bD3D11 = false;
    PFN_xrGetD3D11GraphicsRequirementsKHR pfn_xrGetD3D11GraphicsRequirementsKHR;

    bool m_bDepthComposition = false;

    bool m_bUnboundedReferenceSpace = false;

    bool m_bSpatialAnchor = false;
    PFN_xrCreateSpatialAnchorMSFT pfn_xrCreateSpatialAnchorMSFT;
    PFN_xrCreateSpatialAnchorSpaceMSFT pfn_xrCreateSpatialAnchorSpaceMSFT;
    PFN_xrDestroySpatialAnchorMSFT pfn_xrDestroySpatialAnchorMSFT;

    bool m_bHandInteraction = false;

    bool m_bHandTracking = false;
    PFN_xrCreateHandTrackerEXT pfn_xrCreateHandTrackerEXT;
    PFN_xrDestroyHandTrackerEXT pfn_xrDestroyHandTrackerEXT;
    PFN_xrLocateHandJointsEXT pfn_xrLocateHandJointsEXT;

    bool m_bHandTrackingMesh = false;
    PFN_xrCreateHandMeshSpaceMSFT pfn_xrCreateHandMeshSpaceMSFT;
    PFN_xrUpdateHandMeshMSFT pfn_xrUpdateHandMeshMSFT;

    bool m_bHolographicWindowAttachment = false;

    bool m_bRemoting = false;
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
    PFN_xrRemotingSetContextPropertiesMSFT pfn_xrRemotingSetContextPropertiesMSFT;
    PFN_xrRemotingConnectMSFT pfn_xrRemotingConnectMSFT;
    PFN_xrRemotingDisconnectMSFT pfn_xrRemotingDisconnectMSFT;
    PFN_xrRemotingGetConnectionStateMSFT pfn_xrRemotingGetConnectionStateMSFT;
#endif
  };

  // Instance
  XrInstance m_pInstance = XR_NULL_HANDLE;
  Extensions m_Extensions;
#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
  plUniquePtr<class plOpenXRRemoting> m_pRemoting;
#endif

  // System
  uint64_t m_SystemId = XR_NULL_SYSTEM_ID;

  // Session
  XrSession m_pSession = XR_NULL_HANDLE;
  XrSpace m_pSceneSpace = XR_NULL_HANDLE;
  XrSpace m_pLocalSpace = XR_NULL_HANDLE;
  plEventSubscriptionID m_ExecutionEventsId = 0;
  plEventSubscriptionID m_BeginRenderEventsId = 0;
  plEventSubscriptionID m_GALdeviceEventsId = 0;
  XrDebugUtilsMessengerEXT m_pDebugMessenger = XR_NULL_HANDLE;

  // Graphics plugin
  XrEnvironmentBlendMode m_BlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  XrGraphicsBindingD3D11KHR m_XrGraphicsBindingD3D11{XR_TYPE_GRAPHICS_BINDING_D3D11_KHR};
  XrFormFactor m_FormFactor{XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY};
  XrViewConfigurationType m_PrimaryViewConfigurationType{XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO};

  plGALSwapChainHandle m_hSwapChain;

  // Views
  XrViewState m_ViewState{XR_TYPE_VIEW_STATE};
  XrView m_Views[2];
  bool m_bProjectionChanged = true;
  XrCompositionLayerProjection m_Layer{XR_TYPE_COMPOSITION_LAYER_PROJECTION};
  XrCompositionLayerProjectionView m_ProjectionLayerViews[2];
  XrCompositionLayerDepthInfoKHR m_DepthLayerViews[2];

  // State
  bool m_bSessionRunning = false;
  bool m_bExitRenderLoop = false;
  bool m_bRequestRestart = false;
  bool m_bRenderInProgress = false;
  XrSessionState m_SessionState{XR_SESSION_STATE_UNKNOWN};

  XrFrameWaitInfo m_FrameWaitInfo{XR_TYPE_FRAME_WAIT_INFO};
  XrFrameState m_FrameState{XR_TYPE_FRAME_STATE};
  XrFrameBeginInfo m_FrameBeginInfo{XR_TYPE_FRAME_BEGIN_INFO};

  // XR interface state
  plHMDInfo m_Info;
  mutable plUniquePtr<plOpenXRInputDevice> m_pInput;
  plUniquePtr<plOpenXRSpatialAnchors> m_pAnchors;
  plUniquePtr<plOpenXRHandTracking> m_pHandTracking;

  plCamera* m_pCameraToSynchronize = nullptr;
  plEnum<plXRStageSpace> m_StageSpace;
  plUInt32 m_uiSettingsModificationCounter = 0;
  plViewHandle m_hView;

  plWindowOutputTargetXR* m_pCompanion = nullptr;
  bool m_bPresentDelayed = false;
};
