/* ========================================================================

   (C) Copyright 2023 by Molly Rocket, Inc., All Rights Reserved.

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.

   Please see https://computerenhance.com for more information

   ======================================================================== */

/* ========================================================================
   LISTING 119
   ======================================================================== */

/* NOTE(casey): _CRT_SECURE_NO_WARNINGS is here because otherwise we cannot
   call fopen(). If we replace fopen() with fopen_s() to avoid the warning,
   then the code doesn't compile on Linux anymore, since fopen_s() does not
   exist there.

   What exactly the CRT maintainers were thinking when they made this choice,
   I have no idea. */
#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t b32;

typedef float f32;
typedef double f64;

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#include "listing_0108_platform_metrics.cpp"
#include "listing_0117_virtual_address.cpp"

// Cross-platform memory allocation
static void *AllocateBuffer(u64 Size) {
#ifdef _WIN32
  return VirtualAlloc(0, Size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#else
  void *ptr = mmap(NULL, Size, PROT_READ | PROT_WRITE,
                   MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  return (ptr == MAP_FAILED) ? NULL : ptr;
#endif
}

// Cross-platform memory deallocation
static void FreeBuffer(void *ptr, u64 Size) {
#ifdef _WIN32
  VirtualFree(ptr, 0, MEM_RELEASE);
#else
  munmap(ptr, Size);
#endif
}

int main(void) {
  // NOTE(casey): Since we do not use these functions in this particular build,
  // we reference their pointers here to prevent the compiler from complaining
  // about "unused functions".
  (void)&EstimateCPUTimerFreq;
  (void)&DecomposePointer2MB;
  (void)&DecomposePointer1GB;

  InitializeOSMetrics();

  u64 PageSize = 4096; // NOTE(casey): This may not be the OS page size! It is
                       // merely our testing page size.
  u64 PageCount = 1000;
  u64 TotalSize = PageCount * PageSize;

  u8 *Data = (u8 *)AllocateBuffer(TotalSize);
  if (Data) {
    PrintAsLine("Buffer base: ", DecomposePointer4K(Data));
    printf("\n");

    u64 StartFaultCount = ReadOSPageFaultCount();

    u64 PriorOverFaultCount = 0;
    u64 PriorPageIndex = 0;
    for (u64 PageIndex = 0; PageIndex < PageCount; ++PageIndex) {

      Data[TotalSize - 1 - PageSize * PageIndex] = (u8)PageIndex;
      u64 EndFaultCount = ReadOSPageFaultCount();

      u64 OverFaultCount = (EndFaultCount - StartFaultCount) - PageIndex;

      if (OverFaultCount > PriorOverFaultCount) {
        printf(
            "Page %llu: %llu extra faults (%llu pages since last increase)\n",
            PageIndex, OverFaultCount, (PageIndex - PriorPageIndex));
        if (PageIndex > 0) {
          PrintAsLine("     Previous Pointer: ",
                      DecomposePointer4K(Data + TotalSize - 1 -
                                         PageSize * (PageIndex - 1)));
        }
        PrintAsLine(
            "         This Pointer: ",
            DecomposePointer4K(Data + TotalSize - 1 - PageSize * PageIndex));

        PriorOverFaultCount = OverFaultCount;
        PriorPageIndex = PageIndex;
      }
    }

    FreeBuffer(Data, TotalSize);
  } else {
    fprintf(stderr, "ERROR: Unable to allocate memory\n");
  }

  return 0;
}
