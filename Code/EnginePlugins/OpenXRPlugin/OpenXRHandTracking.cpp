#include <OpenXRPlugin/OpenXRPluginPCH.h>

#include <Core/World/World.h>
#include <Foundation/Profiling/Profiling.h>
#include <GameEngine/XR/StageSpaceComponent.h>
#include <OpenXRPlugin/OpenXRDeclarations.h>
#include <OpenXRPlugin/OpenXRHandTracking.h>
#include <OpenXRPlugin/OpenXRSingleton.h>

PL_IMPLEMENT_SINGLETON(plOpenXRHandTracking);

bool plOpenXRHandTracking::IsHandTrackingSupported(plOpenXR* pOpenXR)
{
  XrSystemHandTrackingPropertiesEXT handTrackingSystemProperties{XR_TYPE_SYSTEM_HAND_TRACKING_PROPERTIES_EXT};
  XrSystemProperties systemProperties{XR_TYPE_SYSTEM_PROPERTIES, &handTrackingSystemProperties};
  XrResult res = xrGetSystemProperties(pOpenXR->m_pInstance, pOpenXR->m_SystemId, &systemProperties);
  if (res == XrResult::XR_SUCCESS)
  {
    return handTrackingSystemProperties.supportsHandTracking;
  }
  return false;
}

plOpenXRHandTracking::plOpenXRHandTracking(plOpenXR* pOpenXR)
  : m_SingletonRegistrar(this)
  , m_pOpenXR(pOpenXR)
{
  for (plUInt32 uiSide : {0, 1})
  {
    const XrHandEXT uiHand = uiSide == 0 ? XR_HAND_LEFT_EXT : XR_HAND_RIGHT_EXT;
    XrHandTrackerCreateInfoEXT createInfo{XR_TYPE_HAND_TRACKER_CREATE_INFO_EXT};
    createInfo.hand = uiHand;
    XR_LOG_ERROR(m_pOpenXR->m_Extensions.pfn_xrCreateHandTrackerEXT(pOpenXR->m_pSession, &createInfo, &m_HandTracker[uiSide]));

    m_Locations[uiSide].type = XR_TYPE_HAND_JOINT_LOCATIONS_EXT;
    m_Locations[uiSide].next = &m_Velocities;
    m_Locations[uiSide].jointCount = XR_HAND_JOINT_COUNT_EXT;
    m_Locations[uiSide].jointLocations = m_JointLocations[uiSide];

    m_Velocities[uiSide].type = XR_TYPE_HAND_JOINT_VELOCITIES_EXT;
    m_Velocities[uiSide].jointCount = XR_HAND_JOINT_COUNT_EXT;
    m_Velocities[uiSide].jointVelocities = m_JointVelocities[uiSide];

    m_JointData[uiSide].SetCount(XR_HAND_JOINT_LITTLE_TIP_EXT + 1);
    for (plUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_EXT; ++i)
    {
      m_JointData[uiSide][i].m_Bone.m_Transform.SetIdentity();
    }
  }

  // Map hand parts to hand joints
  m_HandParts[plXRHandPart::Palm].PushBack(XR_HAND_JOINT_PALM_EXT);
  m_HandParts[plXRHandPart::Palm].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[plXRHandPart::Wrist].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[plXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_TIP_EXT);
  m_HandParts[plXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_DISTAL_EXT);
  m_HandParts[plXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_PROXIMAL_EXT);
  m_HandParts[plXRHandPart::Thumb].PushBack(XR_HAND_JOINT_THUMB_METACARPAL_EXT);
  m_HandParts[plXRHandPart::Thumb].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[plXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_TIP_EXT);
  m_HandParts[plXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_DISTAL_EXT);
  m_HandParts[plXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_INTERMEDIATE_EXT);
  m_HandParts[plXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_PROXIMAL_EXT);
  m_HandParts[plXRHandPart::Index].PushBack(XR_HAND_JOINT_INDEX_METACARPAL_EXT);
  m_HandParts[plXRHandPart::Index].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[plXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_TIP_EXT);
  m_HandParts[plXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_DISTAL_EXT);
  m_HandParts[plXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_INTERMEDIATE_EXT);
  m_HandParts[plXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_PROXIMAL_EXT);
  m_HandParts[plXRHandPart::Middle].PushBack(XR_HAND_JOINT_MIDDLE_METACARPAL_EXT);
  m_HandParts[plXRHandPart::Middle].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[plXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_TIP_EXT);
  m_HandParts[plXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_DISTAL_EXT);
  m_HandParts[plXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_INTERMEDIATE_EXT);
  m_HandParts[plXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_PROXIMAL_EXT);
  m_HandParts[plXRHandPart::Ring].PushBack(XR_HAND_JOINT_RING_METACARPAL_EXT);
  m_HandParts[plXRHandPart::Ring].PushBack(XR_HAND_JOINT_WRIST_EXT);

  m_HandParts[plXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_TIP_EXT);
  m_HandParts[plXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_DISTAL_EXT);
  m_HandParts[plXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_INTERMEDIATE_EXT);
  m_HandParts[plXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_PROXIMAL_EXT);
  m_HandParts[plXRHandPart::Little].PushBack(XR_HAND_JOINT_LITTLE_METACARPAL_EXT);
  m_HandParts[plXRHandPart::Little].PushBack(XR_HAND_JOINT_WRIST_EXT);
}

plOpenXRHandTracking::~plOpenXRHandTracking()
{
  for (plUInt32 uiSide : {0, 1})
  {
    XR_LOG_ERROR(m_pOpenXR->m_Extensions.pfn_xrDestroyHandTrackerEXT(m_HandTracker[uiSide]));
  }
}

plXRHandTrackingInterface::HandPartTrackingState plOpenXRHandTracking::TryGetBoneTransforms(
  plEnum<plXRHand> hand, plEnum<plXRHandPart> handPart, plEnum<plXRTransformSpace> space, plDynamicArray<plXRHandBone>& out_bones)
{
  PL_ASSERT_DEV(handPart <= plXRHandPart::Little, "Invalid hand part.");
  out_bones.Clear();

  for (plUInt32 uiJointIndex : m_HandParts[handPart])
  {
    const JointData& jointData = m_JointData[hand][uiJointIndex];
    if (!jointData.m_bValid)
      return plXRHandTrackingInterface::HandPartTrackingState::Untracked;

    out_bones.PushBack(jointData.m_Bone);
  }

  if (space == plXRTransformSpace::Global)
  {
    plWorld* pWorld = m_pOpenXR->GetWorld();
    if (!pWorld)
      return plXRHandTrackingInterface::HandPartTrackingState::NotSupported;

    if (const plStageSpaceComponentManager* pStageMan = pWorld->GetComponentManager<plStageSpaceComponentManager>())
    {
      if (const plStageSpaceComponent* pStage = pStageMan->GetSingletonComponent())
      {
        const plTransform globalStageTransform = pStage->GetOwner()->GetGlobalTransform();
        for (plXRHandBone& bone : out_bones)
        {
          plTransform local = bone.m_Transform;
          bone.m_Transform = plTransform::MakeGlobalTransform(globalStageTransform, local);
        }
      }
    }
  }
  return plXRHandTrackingInterface::HandPartTrackingState::Tracked;
}

void plOpenXRHandTracking::UpdateJointTransforms()
{
  PL_PROFILE_SCOPE("UpdateJointTransforms");
  const XrTime time = m_pOpenXR->m_FrameState.predictedDisplayTime;
  XrHandJointsLocateInfoEXT locateInfo{XR_TYPE_HAND_JOINTS_LOCATE_INFO_EXT};
  locateInfo.baseSpace = m_pOpenXR->GetBaseSpace();
  locateInfo.time = time;

  for (plUInt32 uiSide : {0, 1})
  {
    if (m_pOpenXR->m_Extensions.pfn_xrLocateHandJointsEXT(m_HandTracker[uiSide], &locateInfo, &m_Locations[uiSide]) != XrResult::XR_SUCCESS)
      m_Locations[uiSide].isActive = false;

    if (m_Locations[uiSide].isActive)
    {
      for (plUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_EXT; ++i)
      {
        const XrHandJointLocationEXT& spaceLocation = m_JointLocations[uiSide][i];
        if ((spaceLocation.locationFlags & XR_SPACE_LOCATION_POSITION_VALID_BIT) != 0 &&
            (spaceLocation.locationFlags & XR_SPACE_LOCATION_ORIENTATION_VALID_BIT) != 0)
        {
          m_JointData[uiSide][i].m_bValid = true;
          m_JointData[uiSide][i].m_Bone.m_fRadius = spaceLocation.radius;
          m_JointData[uiSide][i].m_Bone.m_Transform.m_vPosition = plOpenXR::ConvertPosition(spaceLocation.pose.position);
          m_JointData[uiSide][i].m_Bone.m_Transform.m_qRotation = plOpenXR::ConvertOrientation(spaceLocation.pose.orientation);
        }
        else
        {
          m_JointData[uiSide][i].m_bValid = false;
        }
      }
    }
    else
    {
      for (plUInt32 i = 0; i <= XR_HAND_JOINT_LITTLE_TIP_EXT; ++i)
      {
        m_JointData[uiSide][i].m_bValid = false;
      }
    }
  }
}

PL_STATICLINK_FILE(OpenXRPlugin, OpenXRPlugin_OpenXRHandTracking);
