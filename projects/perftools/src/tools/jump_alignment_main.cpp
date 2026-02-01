/* ========================================================================

   (C) Copyright 2023 by Molly Rocket, Inc., All Rights Reserved.

   This software is provided 'as-is', without any express or implied
   warranty. In no event will the authors be held liable for any damages
   arising from the use of this software.

   Please see https://computerenhance.com for more information

   ======================================================================== */

/* ========================================================================
   LISTING 140 - Jump Alignment Test
   ======================================================================== */

#include "common/repetition_tester.hpp"
#include "common/test_helpers.hpp"
#include "common/types.hpp"

#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef void ASMFunction(u64 Count, u8 *Data);

extern "C" void NOPAligned64(u64 Count, u8 *Data);
extern "C" void NOPAligned1(u64 Count, u8 *Data);
extern "C" void NOPAligned15(u64 Count, u8 *Data);
extern "C" void NOPAligned31(u64 Count, u8 *Data);
// This one is half as fast (4.30 to 2.14 gb/s)
extern "C" void NOPAligned63(u64 Count, u8 *Data);

struct test_function {
  const char *Name;
  ASMFunction *Func;
};

test_function TestFunctions[] = {
    {"NOPAligned64", NOPAligned64}, // Baseline - no extra NOPs
    {"NOPAligned1", NOPAligned1},   // 1 NOP before loop
    {"NOPAligned15", NOPAligned15}, // 15 NOPs before loop
    {"NOPAligned31", NOPAligned31}, // 31 NOPs before loop
    {"NOPAligned63",
     NOPAligned63}, // 63 NOPs before loop (max before 64-byte boundary)
};

int main(void) {
  TestParameters params = {};
  params.bufferSize = 1ULL * 1024 * 1024 * 1024;
  params.allocType = AllocType_Malloc; // Or AllocType_HugePages if desired
  handleAllocation(&params, &params.buffer);

  if (params.buffer) {
    RepetitionTester Testers[ArrayCount(TestFunctions)] = {};
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
