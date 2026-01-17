#pragma once

#include <cstdint>

typedef uint64_t u64;

#if _WIN32

#include <intrin.h>
#include <windows.h>
#include <psapi.h>

inline u64 GetOSTimerFreq(void)
{
	LARGE_INTEGER Freq;
	QueryPerformanceFrequency(&Freq);
	return Freq.QuadPart;
}

inline u64 ReadOSTimer(void)
{
	LARGE_INTEGER Value;
	QueryPerformanceCounter(&Value);
	return Value.QuadPart;
}

// Windows page fault tracking
static HANDLE gProcessHandle = nullptr;

inline void InitPageFaultTracking(void)
{
	if (!gProcessHandle) {
		gProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
	}
}

inline u64 ReadPageFaultCount(void)
{
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

#include <x86intrin.h>
#include <sys/time.h>
#include <sys/resource.h>

inline u64 GetOSTimerFreq(void)
{
	return 1000000;
}

inline u64 ReadOSTimer(void)
{
	struct timeval Value;
	gettimeofday(&Value, 0);
	
	u64 Result = GetOSTimerFreq()*(u64)Value.tv_sec + (u64)Value.tv_usec;
	return Result;
}

// Linux page fault tracking via getrusage
inline void InitPageFaultTracking(void)
{
	// No initialization needed on Linux
}

inline u64 ReadPageFaultCount(void)
{
	struct rusage usage;
	getrusage(RUSAGE_SELF, &usage);
	
	// Major + minor page faults
	return usage.ru_majflt + usage.ru_minflt;
}

#endif

inline u64 ReadCPUTimer(void)
{
	return __rdtsc();
}

inline u64 EstimateCPUFreq(u64 MillisecondsToWait = 100)
{
	u64 OSFreq = GetOSTimerFreq();
	
	u64 CPUStart = ReadCPUTimer();
	u64 OSStart = ReadOSTimer();
	u64 OSEnd = 0;
	u64 OSElapsed = 0;
	u64 OSWaitTime = OSFreq * MillisecondsToWait / 1000;
	while(OSElapsed < OSWaitTime)
	{
		OSEnd = ReadOSTimer();
		OSElapsed = OSEnd - OSStart;
	}
	
	u64 CPUEnd = ReadCPUTimer();
	u64 CPUElapsed = CPUEnd - CPUStart;
	u64 CPUFreq = 0;
	if(OSElapsed)
	{
		CPUFreq = CPUElapsed * OSFreq / OSElapsed;
	}
	
	return CPUFreq;
}
