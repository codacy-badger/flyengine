#include <Core/Input/InputManager.h>
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <Foundation/Logging/Log.h>
#include <XBoxControllerPlugin/InputDeviceXBox.h>

#include <Xinput.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plInputDeviceXBox360, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plInputDeviceXBox360::plInputDeviceXBox360()
{
  for (plInt32 i = 0; i < MaxControllers; ++i)
    m_bControllerConnected[i] = false;
}

plInputDeviceXBox360::~plInputDeviceXBox360() {}

void plInputDeviceXBox360::RegisterControllerButton(const char* szButton, const char* szName, plBitflags<plInputSlotFlags> SlotFlags)
{
  plStringBuilder s, s2;

  for (plInt32 i = 0; i < MaxControllers; ++i)
  {
    s.Format("controller{0}_{1}", i, szButton);
    s2.Format("Cont {0}: {1}", i + 1, szName);
    RegisterInputSlot(s.GetData(), s2.GetData(), SlotFlags);
  }
}

void plInputDeviceXBox360::SetDeadZone(const char* szButton)
{
  plStringBuilder s;

  for (plInt32 i = 0; i < MaxControllers; ++i)
  {
    s.Format("controller{0}_{1}", i, szButton);
    plInputManager::SetInputSlotDeadZone(s.GetData(), 0.23f);
  }
}

void plInputDeviceXBox360::RegisterInputSlots()
{
  RegisterControllerButton("button_a", "Button A", plInputSlotFlags::IsButton);
  RegisterControllerButton("button_b", "Button B", plInputSlotFlags::IsButton);
  RegisterControllerButton("button_x", "Button X", plInputSlotFlags::IsButton);
  RegisterControllerButton("button_y", "Button Y", plInputSlotFlags::IsButton);
  RegisterControllerButton("button_start", "Start", plInputSlotFlags::IsButton);
  RegisterControllerButton("button_back", "Back", plInputSlotFlags::IsButton);
  RegisterControllerButton("left_shoulder", "Left Shoulder", plInputSlotFlags::IsButton);
  RegisterControllerButton("right_shoulder", "Right Shoulder", plInputSlotFlags::IsButton);
  RegisterControllerButton("left_trigger", "Left Trigger", plInputSlotFlags::IsAnalogTrigger);
  RegisterControllerButton("right_trigger", "Right Trigger", plInputSlotFlags::IsAnalogTrigger);
  RegisterControllerButton("pad_up", "Pad Up", plInputSlotFlags::IsDPad);
  RegisterControllerButton("pad_down", "Pad Down", plInputSlotFlags::IsDPad);
  RegisterControllerButton("pad_left", "Pad Left", plInputSlotFlags::IsDPad);
  RegisterControllerButton("pad_right", "Pad Right", plInputSlotFlags::IsDPad);
  RegisterControllerButton("left_stick", "Left Stick", plInputSlotFlags::IsButton);
  RegisterControllerButton("right_stick", "Right Stick", plInputSlotFlags::IsButton);

  RegisterControllerButton("leftstick_negx", "Left Stick Left", plInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("leftstick_posx", "Left Stick Right", plInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("leftstick_negy", "Left Stick Down", plInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("leftstick_posy", "Left Stick Up", plInputSlotFlags::IsAnalogStick);

  RegisterControllerButton("rightstick_negx", "Right Stick Left", plInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("rightstick_posx", "Right Stick Right", plInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("rightstick_negy", "Right Stick Down", plInputSlotFlags::IsAnalogStick);
  RegisterControllerButton("rightstick_posy", "Right Stick Up", plInputSlotFlags::IsAnalogStick);

  SetDeadZone("left_trigger");
  SetDeadZone("right_trigger");
  SetDeadZone("leftstick_negx");
  SetDeadZone("leftstick_posx");
  SetDeadZone("leftstick_negy");
  SetDeadZone("leftstick_posy");
  SetDeadZone("rightstick_negx");
  SetDeadZone("rightstick_posx");
  SetDeadZone("rightstick_negy");
  SetDeadZone("rightstick_posy");

  plLog::Success("Initialized XBox 360 Controller.");
}

const char* szControllerName[] = {
  "controller0_",
  "controller1_",
  "controller2_",
  "controller3_",

  "controller4_",
  "controller5_",
  "controller6_",
  "controller7_",
};

PLASMA_CHECK_AT_COMPILETIME(PLASMA_ARRAY_SIZE(szControllerName) >= plInputDeviceXBox360::MaxControllers);

void plInputDeviceXBox360::SetValue(plInt32 iController, const char* szButton, float fValue)
{
  plStringBuilder s = szControllerName[iController];
  s.Append(szButton);
  float& fVal = m_InputSlotValues[s];
  fVal = plMath::Max(fVal, fValue);
}

void plInputDeviceXBox360::UpdateHardwareState(plTime tTimeDifference)
{
  UpdateVibration(tTimeDifference);
}

void plInputDeviceXBox360::UpdateInputSlotValues()
{
  // reset all keys
  for (auto it = m_InputSlotValues.GetIterator(); it.IsValid(); ++it)
    it.Value() = 0.0f;

  XINPUT_STATE State[MaxControllers];
  bool bIsAvailable[MaxControllers];

  // update not connected controllers only every few milliseconds, apparently it takes quite some time to do this
  // even on not connected controllers
  static plTime tLastControllerSearch;
  const plTime tNow = plTime::Now();
  const bool bSearchControllers = tNow - tLastControllerSearch > plTime::Seconds(0.5);

  if (bSearchControllers)
    tLastControllerSearch = tNow;

  // get the data from all physical devices
  for (plInt32 iPhysical = 0; iPhysical < MaxControllers; ++iPhysical)
  {
    if (bSearchControllers || m_bControllerConnected[iPhysical])
    {
      bIsAvailable[iPhysical] = (XInputGetState(iPhysical, &State[iPhysical]) == ERROR_SUCCESS);

      if (m_bControllerConnected[iPhysical] != bIsAvailable[iPhysical])
      {
        plLog::Info("XBox Controller {0} has been {1}.", iPhysical, bIsAvailable[iPhysical] ? "connected" : "disconnected");

        // this makes sure to reset all values below
        if (!bIsAvailable[iPhysical])
          plMemoryUtils::ZeroFill(&State[iPhysical], 1);
      }
    }
    else
      bIsAvailable[iPhysical] = m_bControllerConnected[iPhysical];
  }

  // now update all virtual controllers
  for (plUInt8 uiVirtual = 0; uiVirtual < MaxControllers; ++uiVirtual)
  {
    // check from which physical device to take the input data
    const plInt8 iPhysical = GetControllerMapping(uiVirtual);

    // if the mapping is negative (which means 'deactivated'), ignore this controller
    if ((iPhysical < 0) || (iPhysical >= MaxControllers))
      continue;

    // if the controller is not active, no point in updating it
    // if it just got inactive, this will reset it once, because the state is only passed on after this loop
    if (!m_bControllerConnected[iPhysical])
      continue;

    SetValue(uiVirtual, "pad_up", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "pad_down", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "pad_left", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "pad_right", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "button_start", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "button_back", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_BACK) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "left_stick", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "right_stick", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "left_shoulder", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "right_shoulder", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "button_a", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "button_b", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_B) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "button_x", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_X) != 0) ? 1.0f : 0.0f);
    SetValue(uiVirtual, "button_y", ((State[iPhysical].Gamepad.wButtons & XINPUT_GAMEPAD_Y) != 0) ? 1.0f : 0.0f);

    const float fTriggerRange = 255.0f;

    SetValue(uiVirtual, "left_trigger", State[iPhysical].Gamepad.bLeftTrigger / fTriggerRange);
    SetValue(uiVirtual, "right_trigger", State[iPhysical].Gamepad.bRightTrigger / fTriggerRange);

    // all input points have dead-zones, so we can let the state handler do the rest
    SetValue(uiVirtual, "leftstick_negx", (State[iPhysical].Gamepad.sThumbLX < 0) ? (-State[iPhysical].Gamepad.sThumbLX / 32767.0f) : 0.0f);
    SetValue(uiVirtual, "leftstick_posx", (State[iPhysical].Gamepad.sThumbLX > 0) ? (State[iPhysical].Gamepad.sThumbLX / 32767.0f) : 0.0f);
    SetValue(uiVirtual, "leftstick_negy", (State[iPhysical].Gamepad.sThumbLY < 0) ? (-State[iPhysical].Gamepad.sThumbLY / 32767.0f) : 0.0f);
    SetValue(uiVirtual, "leftstick_posy", (State[iPhysical].Gamepad.sThumbLY > 0) ? (State[iPhysical].Gamepad.sThumbLY / 32767.0f) : 0.0f);

    SetValue(uiVirtual, "rightstick_negx", (State[iPhysical].Gamepad.sThumbRX < 0) ? (-State[iPhysical].Gamepad.sThumbRX / 32767.0f) : 0.0f);
    SetValue(uiVirtual, "rightstick_posx", (State[iPhysical].Gamepad.sThumbRX > 0) ? (State[iPhysical].Gamepad.sThumbRX / 32767.0f) : 0.0f);
    SetValue(uiVirtual, "rightstick_negy", (State[iPhysical].Gamepad.sThumbRY < 0) ? (-State[iPhysical].Gamepad.sThumbRY / 32767.0f) : 0.0f);
    SetValue(uiVirtual, "rightstick_posy", (State[iPhysical].Gamepad.sThumbRY > 0) ? (State[iPhysical].Gamepad.sThumbRY / 32767.0f) : 0.0f);
  }

  for (plInt32 iPhysical = 0; iPhysical < MaxControllers; ++iPhysical)
    m_bControllerConnected[iPhysical] = bIsAvailable[iPhysical];
}

bool plInputDeviceXBox360::IsControllerConnected(plUInt8 uiPhysical) const
{
  PLASMA_ASSERT_DEV(uiPhysical < MaxControllers, "Invalid Controller Index {0}", uiPhysical);

  return m_bControllerConnected[uiPhysical];
}

void plInputDeviceXBox360::ApplyVibration(plUInt8 uiPhysicalController, Motor::Enum eMotor, float fStrength)
{
  if (!m_bControllerConnected[uiPhysicalController])
    return;

  static XINPUT_VIBRATION v[MaxControllers];

  if (eMotor == Motor::LeftMotor)
    v[uiPhysicalController].wLeftMotorSpeed = (WORD)(fStrength * 65535.0f);

  if (eMotor == Motor::RightMotor)
  {
    v[uiPhysicalController].wRightMotorSpeed = (WORD)(fStrength * 65535.0f);

    XInputSetState(uiPhysicalController, &v[uiPhysicalController]);
  }
}



PLASMA_STATICLINK_FILE(System, System_XBoxController_InputDeviceXBox);
