#pragma once

#include <cstdint>

typedef uint64_t u64;

#if _WIN32

#include <intrin.h>
#include <windows.h>

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

#else

#include <x86intrin.h>
#include <sys/time.h>

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
