#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/AnimationSystem/AnimGraph/AnimGraph.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimNodes/DebugAnimNodes.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogAnimNode, 1, plRTTINoAllocator)
  {
    PL_BEGIN_PROPERTIES
    {
      PL_MEMBER_PROPERTY("Text", m_sText)->AddAttributes(new plDefaultValueAttribute("Values: {0}/{1}-{3}/{4}")),

      PL_MEMBER_PROPERTY("InActivate", m_InActivate)->AddAttributes(new plHiddenAttribute()),
      PL_MEMBER_PROPERTY("NumberCount", m_uiNumberCount)->AddAttributes(new plNoTemporaryTransactionsAttribute(), new plDynamicPinAttribute(), new plDefaultValueAttribute(1)),
      PL_ARRAY_MEMBER_PROPERTY("InNumbers", m_InNumbers)->AddAttributes(new plHiddenAttribute(), new plDynamicPinAttribute("NumberCount")),
    }
    PL_END_PROPERTIES;
    PL_BEGIN_ATTRIBUTES
    {
      new plCategoryAttribute("Debug"),
      new plColorAttribute(plColorScheme::DarkUI(plColorScheme::Pink)),
      new plTitleAttribute("Log: '{Text}'"),
    }
    PL_END_ATTRIBUTES;
  }
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plResult plLogAnimNode::SerializeNode(plStreamWriter& stream) const
{
  stream.WriteVersion(1);

  PL_SUCCEED_OR_RETURN(SUPER::SerializeNode(stream));

  stream << m_sText;
  stream << m_uiNumberCount;

  PL_SUCCEED_OR_RETURN(m_InActivate.Serialize(stream));
  PL_SUCCEED_OR_RETURN(stream.WriteArray(m_InNumbers));

  return PL_SUCCESS;
}

plResult plLogAnimNode::DeserializeNode(plStreamReader& stream)
{
  const auto version = stream.ReadVersion(1);
  PL_IGNORE_UNUSED(version);

  PL_SUCCEED_OR_RETURN(SUPER::DeserializeNode(stream));

  stream >> m_sText;
  stream >> m_uiNumberCount;

  PL_SUCCEED_OR_RETURN(m_InActivate.Deserialize(stream));
  PL_SUCCEED_OR_RETURN(stream.ReadArray(m_InNumbers));

  return PL_SUCCESS;
}

static plStringView BuildFormattedText(plStringView sText, const plVariantArray& params, plStringBuilder& ref_sStorage)
{
  plHybridArray<plString, 12> stringStorage;
  stringStorage.Reserve(params.GetCount());
  for (auto& param : params)
  {
    stringStorage.PushBack(param.ConvertTo<plString>());
  }

  plHybridArray<plStringView, 12> stringViews;
  stringViews.Reserve(stringStorage.GetCount());
  for (auto& s : stringStorage)
  {
    stringViews.PushBack(s);
  }

  plFormatString fs(sText);
  return fs.BuildFormattedText(ref_sStorage, stringViews.GetData(), stringViews.GetCount());
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogInfoAnimNode, 1, plRTTIDefaultAllocator<plLogInfoAnimNode>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Log Info: '{Text}'"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plLogInfoAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  plVariantArray params;
  for (auto& n : m_InNumbers)
  {
    params.PushBack(n.GetNumber(ref_graph));
  }

  plStringBuilder sStorage;
  plLog::Info(BuildFormattedText(m_sText, params, sStorage));
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLogErrorAnimNode, 1, plRTTIDefaultAllocator<plLogErrorAnimNode>)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Log Error: '{Text}'"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plLogErrorAnimNode::Step(plAnimController& ref_controller, plAnimGraphInstance& ref_graph, plTime tDiff, const plSkeletonResource* pSkeleton, plGameObject* pTarget) const
{
  if (!m_InActivate.IsTriggered(ref_graph))
    return;

  plVariantArray params;
  for (auto& n : m_InNumbers)
  {
    params.PushBack(n.GetNumber(ref_graph));
  }

  plStringBuilder sStorage;
  plLog::Error(BuildFormattedText(m_sText, params, sStorage));
}


PL_STATICLINK_FILE(RendererCore, RendererCore_AnimationSystem_AnimGraph_AnimNodes_DebugAnimNodes);
