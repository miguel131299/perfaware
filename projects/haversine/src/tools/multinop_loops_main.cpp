/* ========================================================================

   (C) Copyright 2023 by Molly Rocket, Inc., All Rights Reserved.

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.

   Please see https://computerenhance.com for more information

   ======================================================================== */

/* ========================================================================
   LISTING 135
   ======================================================================== */

/* NOTE(casey): _CRT_SECURE_NO_WARNINGS is here because otherwise we cannot
   call fopen(). If we replace fopen() with fopen_s() to avoid the warning,
   then the code doesn't compile on Linux anymore, since fopen_s() does not
   exist there.

   What exactly the CRT maintainers were thinking when they made this choice,
   I have no idea. */
#define _CRT_SECURE_NO_WARNINGS

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int32_t b32;

typedef float f32;
typedef double f64;

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#include "haversine/repetition_tester.hpp"
#include "haversine/test_helpers.hpp"

typedef void ASMFunction(u64 Count, u8 *Data);

extern "C" void NOP3x1AllBytes(u64 Count, u8 *Data);
extern "C" void NOP1x3AllBytes(u64 Count, u8 *Data);
extern "C" void NOP1x9AllBytes(u64 Count, u8 *Data);

struct test_function {
  const char *Name;
  ASMFunction *Func;
};

test_function TestFunctions[] = {
    // All loops perform the same computation. The difference is how many noops
    // they must decode.
    {"NOP3x1AllBytes", NOP3x1AllBytes},    // Min: 4.37 gb/s
    {"NOP1x3AllBytes", NOP1x3AllBytes},    // Min: 4.33 gb/s
    {"NOP1x9AllBytesASM", NOP1x9AllBytes}, // Min: 2.15 gb/s. This clearly shows
                                           // the decoder bottlenecks and can't
    // send enough microops to the execution.
};

int main(void) {
  TestParameters params = {};
  params.bufferSize = 1ULL * 1024 * 1024 * 1024;
  params.allocType = AllocType_Malloc; // Or AllocType_HugePages if desired
  handleAllocation(&params, &params.buffer);

  if (params.buffer) {
    RepetitionTester Testers[ArrayCount(TestFunctions)];
    for (;;) {
      for (u32 FuncIndex = 0; FuncIndex < ArrayCount(TestFunctions);
           ++FuncIndex) {
        RepetitionTester &Tester = Testers[FuncIndex];
        test_function &TestFunc = TestFunctions[FuncIndex];

        printf("\n--- %s ---\n", TestFunc.Name);
        Tester.newTestWave(params.bufferSize);

        REPETITION_TEST_BEGIN(Tester) {
          REPETITION_TEST_START_TIMING(Tester);
          TestFunc.Func(params.bufferSize, (u8 *)params.buffer);
          REPETITION_TEST_END_TIMING(Tester);
          REPETITION_TEST_COUNT_BYTES(Tester, params.bufferSize);
        }
      }
    }
  } else {
    fprintf(stderr, "Unable to allocate memory buffer for testing");
  }

  handleDeallocation(&params, &params.buffer);
  return 0;
}
