#pragma once

#include "common/types.hpp"
#include <climits>
#include <fcntl.h>
#include <unistd.h>

#if _WIN32

#include <intrin.h>
#include <psapi.h>
#include <windows.h>

inline u64 GetOSTimerFreq(void) {
  LARGE_INTEGER Freq;
  QueryPerformanceFrequency(&Freq);
  return Freq.QuadPart;
}

inline u64 ReadOSTimer(void) {
  LARGE_INTEGER Value;
  QueryPerformanceCounter(&Value);
  return Value.QuadPart;
}

// Windows page fault tracking
static HANDLE gProcessHandle = nullptr;

inline void InitPageFaultTracking(void) {
  if (!gProcessHandle) {
    gProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
                                 FALSE, GetCurrentProcessId());
  }
}

inline u64 ReadPageFaultCount(void) {
  if (!gProcessHandle) {
    InitPageFaultTracking();
  }

  PROCESS_MEMORY_COUNTERS pmc;
  if (GetProcessMemoryInfo(gProcessHandle, &pmc, sizeof(pmc))) {
    return pmc.PageFaultCount;
  }
  return 0;
}

#else

#include <sys/resource.h>
#include <sys/time.h>
#include <x86intrin.h>

inline u64 GetOSTimerFreq(void) { return 1000000; }

inline u64 ReadOSTimer(void) {
  struct timeval Value;
  gettimeofday(&Value, 0);

  u64 Result = GetOSTimerFreq() * (u64)Value.tv_sec + (u64)Value.tv_usec;
  return Result;
}

// Linux page fault tracking via getrusage
inline void InitPageFaultTracking(void) {
  // No initialization needed on Linux
}

inline u64 ReadPageFaultCount(void) {
  struct rusage usage;
  getrusage(RUSAGE_SELF, &usage);

  // ru_minflt  the number of page faults serviced without any I/O activity.
  // ru_majflt  the number of page faults serviced that required I/O activity.
  return usage.ru_majflt + usage.ru_minflt;
}

#endif

inline u64 ReadCPUTimer(void) { return __rdtsc(); }

inline u64 EstimateCPUFreq(u64 MillisecondsToWait = 100) {
  u64 OSFreq = GetOSTimerFreq();

  u64 CPUStart = ReadCPUTimer();
  u64 OSStart = ReadOSTimer();
  u64 OSEnd = 0;
  u64 OSElapsed = 0;
  u64 OSWaitTime = OSFreq * MillisecondsToWait / 1000;
  while (OSElapsed < OSWaitTime) {
    OSEnd = ReadOSTimer();
    OSElapsed = OSEnd - OSStart;
  }

  u64 CPUEnd = ReadCPUTimer();
  u64 CPUElapsed = CPUEnd - CPUStart;
  u64 CPUFreq = 0;
  if (OSElapsed) {
    CPUFreq = CPUElapsed * OSFreq / OSElapsed;
  }

  return CPUFreq;
}

static u64 GetMaxOSRandomCount(void) { return SSIZE_MAX; }

static b32 ReadOSRandomBytes(u64 Count, u8 *Dest) {
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd < 0)
    return 0;
  while (Count > 0) {
    ssize_t n = read(fd, Dest, Count);
    if (n <= 0) {
      close(fd);
      return 0;
    }
    Dest += n;
    Count -= n;
  }
  close(fd);
  return 1;
}

inline void FillWithRandomBytes(u64 Count, u8 *Dest) {
  u64 MaxRandCount = GetMaxOSRandomCount();
  u64 AtOffset = 0;
  while (AtOffset < Count) {
    u64 ReadCount = Count - AtOffset;
    if (ReadCount > MaxRandCount) {
      ReadCount = MaxRandCount;
    }

    ReadOSRandomBytes(ReadCount, Dest + AtOffset);
    AtOffset += ReadCount;
  }
}
