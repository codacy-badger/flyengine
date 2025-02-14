#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Tracks/EventTrack.h>
#include <GuiFoundation/Widgets/EventTrackEditData.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEventTrackControlPointData, 1, plRTTIDefaultAllocator<plEventTrackControlPointData>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Tick", m_iTick),
    PL_ACCESSOR_PROPERTY("Event", GetEventName, SetEventName),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plEventTrackData, 3, plRTTIDefaultAllocator<plEventTrackData>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ARRAY_MEMBER_PROPERTY("ControlPoints", m_ControlPoints),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plEventTrackControlPointData::SetTickFromTime(plTime time, plInt64 iFps)
{
  const plUInt32 uiTicksPerStep = 4800 / iFps;
  m_iTick = (plInt64)plMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

plInt64 plEventTrackData::TickFromTime(plTime time) const
{
  const plUInt32 uiTicksPerStep = 4800 / m_uiFramesPerSecond;
  return (plInt64)plMath::RoundToMultiple(time.GetSeconds() * 4800.0, (double)uiTicksPerStep);
}

void plEventTrackData::ConvertToRuntimeData(plEventTrack& out_result) const
{
  out_result.Clear();

  for (const auto& cp : m_ControlPoints)
  {
    out_result.AddControlPoint(cp.GetTickAsTime(), cp.m_sEvent);
  }
}

void plEventSet::AddAvailableEvent(plStringView sEvent)
{
  if (sEvent.IsEmpty())
    return;

  if (m_AvailableEvents.Contains(sEvent))
    return;

  m_bModified = true;
  m_AvailableEvents.Insert(sEvent);
}

plResult plEventSet::WriteToDDL(const char* szFile)
{
  plDeferredFileWriter file;
  file.SetOutput(szFile);

  plOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  for (const auto& s : m_AvailableEvents)
  {
    ddl.BeginObject("Event", s.GetData());
    ddl.EndObject();
  }

  if (file.Close().Succeeded())
  {
    m_bModified = false;
    return PL_SUCCESS;
  }

  return PL_FAILURE;
}

plResult plEventSet::ReadFromDDL(const char* szFile)
{
  m_AvailableEvents.Clear();

  plFileReader file;
  if (file.Open(szFile).Failed())
    return PL_FAILURE;

  plOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return PL_FAILURE;

  auto* pRoot = ddl.GetRootElement();

  for (auto* pChild = pRoot->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Event"))
    {
      AddAvailableEvent(pChild->GetName());
    }
  }

  m_bModified = false;
  return PL_SUCCESS;
}
