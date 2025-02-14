#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <Foundation/Math/Size.h>
#include <Foundation/Reflection/Reflection.h>

struct plHMDInfo
{
  plString m_sDeviceName;
  plString m_sDeviceDriver;
  plSizeU32 m_vEyeRenderTargetSize;
};

/// \brief Defines the stage space used for the XR experience.
///
/// This value is set by the plStageSpaceComponent singleton and
/// has to be taken into account by the XR implementation.
struct plXRStageSpace
{
  using StorageType = plUInt8;
  enum Enum : plUInt8
  {
    Seated,   ///< Tracking poses will be relative to a seated head position
    Standing, ///< Tracking poses will be relative to the center of the stage space at ground level.
    Default = Standing,
  };
};
PL_DECLARE_REFLECTABLE_TYPE(PL_GAMEENGINE_DLL, plXRStageSpace);

struct plXRTransformSpace
{
  using StorageType = plUInt8;
  enum Enum : plUInt8
  {
    Local,  ///< Sets the local transform to the pose in stage space. Use if owner is direct child of plStageSpaceComponent.
    Global, ///< Uses the global transform of the device in world space.
    Default = Local,
  };
};
PL_DECLARE_REFLECTABLE_TYPE(PL_GAMEENGINE_DLL, plXRTransformSpace);

struct plXRDeviceType
{
  using StorageType = plUInt8;
  enum Enum : plUInt8
  {
    HMD,
    LeftController,
    RightController,
    DeviceID0,
    DeviceID1,
    DeviceID2,
    DeviceID3,
    DeviceID4,
    DeviceID5,
    DeviceID6,
    DeviceID7,
    DeviceID8,
    DeviceID9,
    DeviceID10,
    DeviceID11,
    DeviceID12,
    DeviceID13,
    DeviceID14,
    DeviceID15,
    Default = HMD,
  };
};
PL_DECLARE_REFLECTABLE_TYPE(PL_GAMEENGINE_DLL, plXRDeviceType);

using plXRDeviceID = plInt8;

/// \brief A device's pose state.
///
/// All values are relative to the stage space of the device,
/// which is controlled by the plStageSpaceComponent singleton and
/// has to be taken into account by the XR implementation.
struct PL_GAMEENGINE_DLL plXRDeviceState
{
  plXRDeviceState();

  plVec3 m_vGripPosition;
  plQuat m_qGripRotation;

  plVec3 m_vAimPosition;
  plQuat m_qAimRotation;

  plEnum<plXRDeviceType> m_Type;
  bool m_bGripPoseIsValid = false;
  bool m_bAimPoseIsValid = false;
  bool m_bDeviceIsConnected = false;
};

/// \brief Defines features the given device supports.
struct plXRDeviceFeatures
{
  using StorageType = plUInt32;
  enum Enum : plUInt32
  {
    None = 0,
    Trigger = PL_BIT(0),                   ///< Float input. If fully pressed, will also trigger 'Select'.
    Select = PL_BIT(1),                    ///< Bool input.
    Menu = PL_BIT(2),                      ///< Bool input.
    Squeple = PL_BIT(3),                   ///< Bool input.
    PrimaryAnalogStick = PL_BIT(4),        ///< 2D axis input.
    PrimaryAnalogStickClick = PL_BIT(5),   ///< Bool input.
    PrimaryAnalogStickTouch = PL_BIT(6),   ///< Bool input.
    SecondaryAnalogStick = PL_BIT(7),      ///< 2D axis input.
    SecondaryAnalogStickClick = PL_BIT(8), ///< Bool input.
    SecondaryAnalogStickTouch = PL_BIT(9), ///< Bool input.
    GripPose = PL_BIT(10),                 ///< 3D Pose input.
    AimPose = PL_BIT(11),                  ///< 3D Pose input.
    Default = None
  };

  struct Bits
  {
    StorageType Trigger : 1;
    StorageType Select : 1;
    StorageType Menu : 1;
    StorageType Squeple : 1;
    StorageType PrimaryAnalogStick : 1;
    StorageType PrimaryAnalogStickClick : 1;
    StorageType PrimaryAnalogStickTouch : 1;
    StorageType SecondaryAnalogStick : 1;
    StorageType SecondaryAnalogStickClick : 1;
    StorageType SecondaryAnalogStickTouch : 1;
    StorageType GripPose : 1;
    StorageType AimPose : 1;
  };
};
PL_DECLARE_FLAGS_OPERATORS(plXRDeviceFeatures);


struct plXRDeviceEventData
{
  enum class Type : plUInt8
  {
    DeviceAdded,
    DeviceRemoved,
  };

  Type m_Type;
  plXRDeviceID uiDeviceID = 0;
};

using plXRDeviceEvent = plEvent<const plXRDeviceEventData&>;
