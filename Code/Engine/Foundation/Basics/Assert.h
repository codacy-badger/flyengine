#pragma once

/// \file

#include <Foundation/Basics.h>

/// ***** Assert Usage Guidelines *****
///
/// For your typical code, use PL_ASSERT_DEV to check that vital preconditions are met.
/// Be aware that PL_ASSERT_DEV is removed in non-development builds (ie. when PL_COMPILE_FOR_DEVELOPMENT is disabled),
/// INCLUDING your code in the assert condition.
/// If the code that you are checking must be executed, even in non-development builds, use PL_VERIFY instead.
/// PL_ASSERT_DEV and PL_VERIFY will trigger a breakpoint in debug builds, but will not interrupt the application
/// in release builds.
///
/// For conditions that are rarely violated or checking is very costly, use PL_ASSERT_DEBUG. This assert is only active
/// in debug builds. This allows to have extra checking while debugging a program, but not waste performance when a
/// development or release build is used.
///
/// If you need to check something that is so vital that the application can only fail (i.e. crash), if that condition
/// is not met, even in release builds, then use PL_ASSERT_RELEASE. This should not be used in frequently executed code,
/// as it is not stripped from non-development builds by default.
///
/// If you need to squeple the last bit of performance out of your code, PL_ASSERT_RELEASE can be disabled, by defining
/// PL_DISABLE_RELEASE_ASSERTS.
/// Please be aware that PL_ASSERT_RELEASE works like the other asserts, i.e. once it is deactivated, the code in the condition
/// is not executed anymore.
///



/// \brief Assert handler callback. Should return true to trigger a break point or false if the assert should be ignored
using plAssertHandler = bool (*)(const char* szSourceFile, plUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg);

PL_FOUNDATION_DLL bool plDefaultAssertHandler(const char* szSourceFile, plUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg);

/// \brief Gets the current assert handler. The default assert handler shows a dialog on windows or prints to the console on other platforms.
PL_FOUNDATION_DLL plAssertHandler plGetAssertHandler();

/// \brief Sets the assert handler. It is the responsibility of the user to chain assert handlers if needed.
PL_FOUNDATION_DLL void plSetAssertHandler(plAssertHandler handler);

/// \brief Called by the assert macros whenever a check failed. Returns true if the user wants to trigger a break point
PL_FOUNDATION_DLL bool plFailedCheck(const char* szSourceFile, plUInt32 uiLine, const char* szFunction, const char* szExpression, const class plFormatString& msg);
PL_FOUNDATION_DLL bool plFailedCheck(const char* szSourceFile, plUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szMsg);

/// \brief Dummy version of plFmt that only takes a single argument
inline const char* plFmt(const char* szFormat)
{
  return szFormat;
}

#if PL_ENABLED(PL_COMPILER_MSVC)
// Hides the call to __debugbreak from MSVCs optimizer to work around a bug in VS 2019
// that can lead to code (memcpy) after an assert to be omitted
PL_FOUNDATION_DLL void MSVC_OutOfLine_DebugBreak(...);
#endif

#ifdef BUILDSYSTEM_CLANG_TIDY
[[noreturn]] void ClangTidyDoNotReturn();
#  define PL_REPORT_FAILURE(szErrorMsg, ...) ClangTidyDoNotReturn()
#else
/// \brief Macro to report a failure when that code is reached. This will ALWAYS be executed, even in release builds, therefore might crash the
/// application (or trigger a debug break).
#  define PL_REPORT_FAILURE(szErrorMsg, ...)                                                                       \
    do                                                                                                             \
    {                                                                                                              \
      if (plFailedCheck(PL_SOURCE_FILE, PL_SOURCE_LINE, PL_SOURCE_FUNCTION, "", plFmt(szErrorMsg, ##__VA_ARGS__))) \
        PL_DEBUG_BREAK;                                                                                            \
    } while (false)
#endif

#ifdef BUILDSYSTEM_CLANG_TIDY
#  define PL_ASSERT_ALWAYS(bCondition, szErrorMsg, ...) \
    do                                                  \
    {                                                   \
      if (!!(bCondition) == false)                      \
        ClangTidyDoNotReturn();                         \
    } while (false)

#  define PL_ANALYSIS_ASSUME(bCondition) PL_ASSERT_ALWAYS(bCondition, "")
#else
/// \brief Macro to raise an error, if a condition is not met. Allows to write a message using plFormatString style. This assert will be triggered, even in
/// non-development builds and cannot be deactivated.
#  define PL_ASSERT_ALWAYS(bCondition, szErrorMsg, ...)                                                                       \
    do                                                                                                                        \
    {                                                                                                                         \
      PL_MSVC_ANALYSIS_WARNING_PUSH                                                                                           \
      PL_MSVC_ANALYSIS_WARNING_DISABLE(6326) /* disable static analysis for the comparison */                                 \
      if (!!(bCondition) == false)                                                                                            \
      {                                                                                                                       \
        if (plFailedCheck(PL_SOURCE_FILE, PL_SOURCE_LINE, PL_SOURCE_FUNCTION, #bCondition, plFmt(szErrorMsg, ##__VA_ARGS__))) \
          PL_DEBUG_BREAK;                                                                                                     \
      }                                                                                                                       \
      PL_MSVC_ANALYSIS_WARNING_POP                                                                                            \
    } while (false)

/// \brief Macro to inform the static analysis that the given condition can be assumed to be true. Useful to give additional information to
/// static analysis if it can't figure it out by itself. Will do nothing outside of static analysis runs.
#  define PL_ANALYSIS_ASSUME(bCondition)
#endif

/// \brief This type of assert can be used to mark code as 'not (yet) implemented' and makes it easier to find it later on by just searching for these
/// asserts.
#define PL_ASSERT_NOT_IMPLEMENTED PL_REPORT_FAILURE("Not implemented");

// Occurrences of PL_ASSERT_DEBUG are compiled out in non-debug builds
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
/// \brief Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using plFormatString style.
/// Compiled out in non-debug builds.
/// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
#  define PL_ASSERT_DEBUG PL_ASSERT_ALWAYS
#else
/// \brief Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using plFormatString style.
/// Compiled out in non-debug builds.
/// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
#  define PL_ASSERT_DEBUG(bCondition, szErrorMsg, ...)
#endif


// Occurrences of PL_ASSERT_DEV are compiled out in non-development builds
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT) || PL_ENABLED(PL_COMPILE_FOR_DEBUG)

/// \brief Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using plFormatString style.
/// Compiled out in non-development builds.
/// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
#  define PL_ASSERT_DEV PL_ASSERT_ALWAYS

/// \brief Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using plFormatString style.
/// Compiled out in non-development builds, however the condition is always evaluated,
/// so you may execute important code in it.
#  define PL_VERIFY PL_ASSERT_ALWAYS

#else

/// \brief Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using plFormatString style.
/// Compiled out in non-development builds.
/// The condition is not evaluated, when this is compiled out, so do not execute important code in it.
#  define PL_ASSERT_DEV(bCondition, szErrorMsg, ...)

/// \brief Macro to raise an error, if a condition is not met.
///
/// Allows to write a message using plFormatString style.
/// Compiled out in non-development builds, however the condition is always evaluated,
/// so you may execute important code in it.
#  define PL_VERIFY(bCondition, szErrorMsg, ...)                             \
    if (!!(bCondition) == false)                                             \
    { /* The condition is evaluated, even though nothing is done with it. */ \
    }

#endif

#if PL_DISABLE_RELEASE_ASSERTS

/// \brief An assert to check conditions even in release builds.
///
/// These asserts can be disabled (and then their condition will not be evaluated),
/// but this needs to be specifically done by the user by defining PL_DISABLE_RELEASE_ASSERTS.
/// That should only be done, if you are intending to ship a product, and want get rid of all unnecessary overhead.
#  define PL_ASSERT_RELEASE(bCondition, szErrorMsg, ...)

#else

/// \brief An assert to check conditions even in release builds.
///
/// These asserts can be disabled (and then their condition will not be evaluated),
/// but this needs to be specifically done by the user by defining PL_DISABLE_RELEASE_ASSERTS.
/// That should only be done, if you are intending to ship a product, and want get rid of all unnecessary overhead.
#  define PL_ASSERT_RELEASE PL_ASSERT_ALWAYS

#endif

/// \brief Macro to make unhandled cases in a switch block an error.
#define PL_DEFAULT_CASE_NOT_IMPLEMENTED \
  default:                              \
    PL_ASSERT_NOT_IMPLEMENTED           \
    break;
