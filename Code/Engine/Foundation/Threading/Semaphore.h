#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Threading/Implementation/ThreadingDeclarations.h>

/// \brief A semaphore is used to synchronize threads, similar to a mutex (see plMutex).
///
/// There are three main differences to a mutex:
/// 1. The thread that acquires a token from a semaphore and the one that returns a token, don't have to be the same.
/// 2. A semaphore can be locked up to N times by multiple threads in parallel. So it can be used to implement constructs like single-writer / multiple/readers.
/// 3. A 'named' semaphore can be opened by other processes as well, which allows you to implement the basics for inter process communication (IPC).
///
/// Semaphores are quite a bit slower than mutexes (10x or so), so don't use them unless you need the added flexibility.
///
/// \sa plMutex, plConditionVariable
class PL_FOUNDATION_DLL plSemaphore
{
  PL_DISALLOW_COPY_AND_ASSIGN(plSemaphore);

public:
  plSemaphore();
  ~plSemaphore();

  /// \brief Attempts to create a new semaphore with an initial number of available tokens.
  ///
  /// If szSharedName is a non-empty string, a 'named' semaphore is created, which can be opened on other processes as well.
  ///
  /// This call can fail, if a semaphore with the same name already exists. Use plSemaphore::Open() instead.
  plResult Create(plUInt32 uiInitialTokenCount = 0, plStringView sSharedName = plStringView());

  /// \brief Attempts to open an existing named semaphore.
  ///
  /// Fails if no such semaphore exists.
  plResult Open(plStringView sSharedName);

  /// \brief Waits until a token is available and acquires it.
  ///
  /// Use TryAcquireToken() to prevent blocking if desired.
  /// AcquireToken() and ReturnToken() may be called from different threads.
  void AcquireToken();

  /// \brief Returns a single token. If another thread is currently waiting for a token, this will wake it up.
  ///
  /// AcquireToken() and ReturnToken() may be called from different threads.
  void ReturnToken();

  /// \brief Same as AcquireToken() but returns immediately with PL_FAILURE, if currently not tokens are available.
  plResult TryAcquireToken();

private:
  plSemaphoreHandle m_hSemaphore = {};
};
