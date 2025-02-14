#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Profiling/Profiling.h>

#if PL_ENABLED(PL_USE_PROFILING)

struct GPUTimingScope
{
  PL_DECLARE_POD_TYPE();

  plGALTimestampHandle m_BeginTimestamp;
  plGALTimestampHandle m_EndTimestamp;
  char m_szName[48];
};

class GPUProfilingSystem
{
public:
  static void ProcessTimestamps(const plGALDeviceEvent& e)
  {
    if (e.m_Type != plGALDeviceEvent::AfterEndFrame)
      return;

    while (!s_TimingScopes.IsEmpty())
    {
      auto& timingScope = s_TimingScopes.PeekFront();

      plTime endTime;
      if (e.m_pDevice->GetTimestampResult(timingScope.m_EndTimestamp, endTime).Succeeded())
      {
        plTime beginTime;
        PL_VERIFY(e.m_pDevice->GetTimestampResult(timingScope.m_BeginTimestamp, beginTime).Succeeded(),
          "Begin timestamp should be finished before end timestamp");

        if (!beginTime.IsZero() && !endTime.IsZero())
        {
#  if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
          static bool warnOnRingBufferOverun = true;
          if (warnOnRingBufferOverun && endTime < beginTime)
          {
            warnOnRingBufferOverun = false;
            plLog::Error("Profiling end is before start, the DX11 timestamp ring buffer was probably overrun.");
          }
#  endif
          plProfilingSystem::AddGPUScope(timingScope.m_szName, beginTime, endTime);
        }

        s_TimingScopes.PopFront();
      }
      else
      {
        // Timestamps are not available yet
        break;
      }
    }
  }

  static GPUTimingScope& AllocateScope() { return s_TimingScopes.ExpandAndGetRef(); }

private:
  static void OnEngineStartup() { plGALDevice::GetDefaultDevice()->s_Events.AddEventHandler(&GPUProfilingSystem::ProcessTimestamps); }

  static void OnEngineShutdown()
  {
    s_TimingScopes.Clear();
    plGALDevice::GetDefaultDevice()->s_Events.RemoveEventHandler(&GPUProfilingSystem::ProcessTimestamps);
  }

  static plDeque<GPUTimingScope, plStaticsAllocatorWrapper> s_TimingScopes;

  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererFoundation, GPUProfilingSystem);
};

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, GPUProfilingSystem)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    GPUProfilingSystem::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    GPUProfilingSystem::OnEngineShutdown();
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

plDeque<GPUTimingScope, plStaticsAllocatorWrapper> GPUProfilingSystem::s_TimingScopes;

//////////////////////////////////////////////////////////////////////////

GPUTimingScope* plProfilingScopeAndMarker::Start(plGALCommandEncoder* pCommandEncoder, const char* szName)
{
  pCommandEncoder->PushMarker(szName);

  auto& timingScope = GPUProfilingSystem::AllocateScope();
  timingScope.m_BeginTimestamp = pCommandEncoder->InsertTimestamp();
  plStringUtils::Copy(timingScope.m_szName, PL_ARRAY_SIZE(timingScope.m_szName), szName);

  return &timingScope;
}

void plProfilingScopeAndMarker::Stop(plGALCommandEncoder* pCommandEncoder, GPUTimingScope*& ref_pTimingScope)
{
  pCommandEncoder->PopMarker();
  ref_pTimingScope->m_EndTimestamp = pCommandEncoder->InsertTimestamp();
  ref_pTimingScope = nullptr;
}

plProfilingScopeAndMarker::plProfilingScopeAndMarker(plGALCommandEncoder* pCommandEncoder, const char* szName)
  : plProfilingScope(szName, nullptr, plTime::MakeZero())
  , m_pCommandEncoder(pCommandEncoder)
{
  m_pTimingScope = Start(pCommandEncoder, szName);
}

plProfilingScopeAndMarker::~plProfilingScopeAndMarker()
{
  Stop(m_pCommandEncoder, m_pTimingScope);
}

#endif

PL_STATICLINK_FILE(RendererFoundation, RendererFoundation_Profiling_Implementation_Profiling);
