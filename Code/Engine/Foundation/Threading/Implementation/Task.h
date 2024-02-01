#pragma once

#include <Foundation/Strings/String.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Types/RefCounted.h>

/// \brief Base class for custom tasks.
class PL_FOUNDATION_DLL plTask : public plRefCounted
{
  PL_DISALLOW_COPY_AND_ASSIGN(plTask);

public:
  plTask();
  virtual ~plTask();

  /// \brief Sets the most important task properties. This has to be done before the task is added to a task group for the first time.
  ///
  /// \param szTaskName
  ///  Will be displayed in profiling tools and is useful for debugging.
  ///
  /// \param nestingMode
  /// See plTaskNesting
  ///
  /// \param Callback
  /// A callback to execute when the task is finished (or canceled).
  /// The most common use case for this is to deallocate the task at that time.
  void ConfigureTask(const char* szTaskName, plTaskNesting nestingMode, plOnTaskFinishedCallback callback = plOnTaskFinishedCallback()); // [tested]

  /// \brief Changes the multiplicity of this task.
  ///
  /// This has to be set before the task is scheduled, ie. before the task group that the task belongs to
  /// has all its dependencies fulfilled and has its tasks queued for execution.
  /// It is allowed to change the multiplicity after the task is added to the plTaskSystem, as long
  /// as the calling code guarantees to set this value in time.
  ///
  /// A task that has a multiplicity of zero (the default) will have its Execute() function called exactly once.
  /// A task with a multiplicity of N will have its ExecuteWithMultiplicity() function called exactly N times,
  /// potentially in parallel on multiple threads.
  /// Since N can be dynamically decided each frame, one can dynamically scale the amount of parallelism
  /// according to the workload.
  void SetMultiplicity(plUInt32 uiMultiplicity); // [tested]

  /// \sa SetMultiplicity
  plUInt32 GetMultiplicity() const { return m_uiMultiplicity; } // [tested]

  /// \brief Returns whether the task has been finished. This includes being canceled.
  ///
  /// \note This function is only reliable when you KNOW that the task has not been reused.
  /// So that limits its usage to the time frame while the task is in use, and it should only
  /// be queried by code that actually manages when to reuse the task.
  /// If other code needs to be able to check whether a task is finished, you should give it
  /// the plTaskGroupID of the task's group. That one can be used to query whether the group
  /// has finished, even minutes later.
  bool IsTaskFinished() const { return m_iRemainingRuns == 0; } // [tested]

  /// \brief Can be used inside an overridden 'Execute' function to terminate execution prematurely.
  bool HasBeenCanceled() const { return m_bCancelExecution; } // [tested]

protected:
  /// \brief Override this to implement the task's supposed functionality.
  ///
  /// This function is called for tasks that do not use multiplicity.
  /// They are executed a single time for each time they are added to the plTaskSystem.
  virtual void Execute() {} // [tested]

  /// \brief Override this to implement the task's supposed functionality.
  ///
  /// This function is called for tasks that use multiplicity.
  /// A task that uses multiplicity is automatically run N times by the plTaskSystem,
  /// each time with an increasing invocation index. This allows to have a single task
  /// to handle something, but then decide dynamically how often to execute it, to subdivide
  /// the workload into multiple pieces.
  /// Since the same task is executed multiple times in parallel, tasks with multiplicity should
  /// not have any mutable state, which is why this function is const.
  virtual void ExecuteWithMultiplicity(plUInt32 uiInvocation) const {} // [tested]

private:
  // The task system and its worker threads implement most of the functionality of the task handling.
  // Therefore they are allowed to modify all this internal state.
  friend class plTaskSystem;

  void Reset();

  /// \brief Called by plTaskSystem to execute the task. Calls 'Execute' internally.
  void Run(plUInt32 uiInvocation);

  /// \brief Decremented when a task is finished, set to zero when canceled.
  plAtomicInteger32 m_iRemainingRuns;

  /// \brief Set to true when the task is SUPPOSED to cancel. Whether the task is able to do that, depends on its implementation.
  bool m_bCancelExecution = false;

  /// \brief Whether this task has been scheduled for execution already, or is still waiting for dependencies to finish.
  bool m_bTaskIsScheduled = false;

  /// \brief Double buffers the state whether this task uses multiplicity, since it can't read m_uiMultiplicity while the task is scheduled.
  bool m_bUsesMultiplicity = false;

  plUInt32 m_uiMultiplicity = 0;

  /// \brief Whether this task may wait (indirectly) on other tasks. See plTaskNesting.
  plTaskNesting m_NestingMode = plTaskNesting::Maybe;

  /// \brief Optional callback to be fired when the task has finished or was canceled.
  plOnTaskFinishedCallback m_OnTaskFinished;

  /// \brief The parent group to which this task belongs.
  plTaskGroupID m_BelongsToGroup;

  plString m_sTaskName;
};
