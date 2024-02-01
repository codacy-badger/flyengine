#include <Foundation/FoundationInternal.h>
PL_FOUNDATION_INTERNAL_HEADER

#include <Foundation/Time/Time.h>

#include <time.h>

void plTime::Initialize()
{
}

plTime plTime::Now()
{
  struct timespec sp;
  clock_gettime(CLOCK_MONOTONIC_RAW, &sp);

  return plTime::MakeFromSeconds((double)sp.tv_sec + (double)(sp.tv_nsec / 1000000000.0));
}
