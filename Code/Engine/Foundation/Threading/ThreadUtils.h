#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

struct plTime;
class plThread;

/// \brief Contains general thread functions.
class PL_FOUNDATION_DLL plThreadUtils
{
public:
  /// \brief Suspends execution of the current thread.
  static void YieldTimeSlice();

  /// \brief Give resources to other hardware threads on the same processor. Does nothing if the processor has no hardware threads.
  static void YieldHardwareThread();

  /// \brief Suspends the execution of the current thread for the given amount of time. (Precision may vary according to OS)
  static void Sleep(const plTime& duration); // [tested]

  /// \brief Helper function to check if the current thread is the main thread (e.g. the thread which initialized the foundation library)
  static bool IsMainThread();

  /// \brief Returns an identifier for the currently running thread.
  static plThreadID GetCurrentThreadID();

private:
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(Foundation, ThreadUtils);

  /// \brief Initialization functionality of the threading system (called by foundation startup and thus private)
  static void Initialize();
};
