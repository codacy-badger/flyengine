#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/CommonMessages.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/EventAnimNode.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSendEventAnimNode, 1, plRTTIDefaultAllocator<plSendEventAnimNode>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("EventName", GetEventName, SetEventName),

    PL_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new plHiddenAttribute()),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Events"),
    new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Orange)),
    new plTitleAttribute("Send Event: '{EventName}'"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plSendEventAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sEventName;

  PL_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));

  return PL_SUCCESS;
}

plResult plSendEventAnimNode::DeserializeNode(plStreamReader& stream)
{
  stream.ReadVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sEventName;

  PL_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));

  return PL_SUCCESS;
}

void plSendEventAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (m_sEventName.IsEmpty())
    return;

  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  plMsgGenericEvent msg;
  msg.m_sMessage = m_sEventName;

  pTarget->SendEventMessage(msg, nullptr);
}


PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_EventAnimNode);
