#include "common/repetition_tester.hpp"
#include "common/test_helpers.hpp"
#include "common/types.hpp"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef void ASMFunction(u64 Count, u8 *Data);

extern "C" void ConditionalNOP(u64 Count, u8 *Data);

struct test_function {
  const char *Name;
  ASMFunction *Func;
};

test_function TestFunctions[] = {{"ConditionalNOP", ConditionalNOP}};

enum branch_pattern {
  BranchPattern_NeverTaken,  // 4.29gb/s
  BranchPattern_AlwaysTaken, // 2.12gb/s
  BranchPattern_Every2,      // 2.80gb/s
  BranchPattern_Every3,      // 1.78gb/s
  BranchPattern_Every4,      // 3.31gb/s
  BranchPattern_CRTRandom,   // 0.33gb/s
  BranchPattern_OSRandom,    // 0.34gb/s

  BranchPattern_Count,
};

static char const *FillWithBranchPattern(branch_pattern Pattern, u64 Count,
                                         u8 *Buffer) {
  char const *PatternName = "UNKNOWN";

  if (Pattern == BranchPattern_OSRandom) {
    PatternName = "OSRandom";
    FillWithRandomBytes(Count, Buffer);
  } else {
    for (u64 Index = 0; Index < Count; ++Index) {
      u8 Value = 0;

      switch (Pattern) {
      case BranchPattern_NeverTaken: {
        PatternName = "Never Taken";
        Value = 0;
      } break;

      case BranchPattern_AlwaysTaken: {
        PatternName = "AlwaysTaken";
        Value = 1;
      } break;

      case BranchPattern_Every2: {
        PatternName = "Every 2";
        Value = ((Index % 2) == 0);
      } break;

      case BranchPattern_Every3: {
        PatternName = "Every 3";
        Value = ((Index % 3) == 0);
      } break;

      case BranchPattern_Every4: {
        PatternName = "Every 4";
        Value = ((Index % 4) == 0);
      } break;

      case BranchPattern_CRTRandom: {
        PatternName = "CRTRandom";
        // NOTE(casey): rand() actually isn't all that random, so, keep in mind
        // that in the future we will look at better ways to get entropy for
        // testing purposes!
        Value = (u8)rand();
      } break;

      default: {
        fprintf(stderr, "Unrecognized branch pattern.\n");
      } break;
      }

      Buffer[Index] = Value;
    }
  }

  return PatternName;
}

int main(void) {
  TestParameters params = {};
  params.bufferSize = 1ULL * 1024 * 1024 * 1024 + 8;
  params.allocType = AllocType_Malloc; // Or AllocType_HugePages if desired
  handleAllocation(&params, &params.buffer);

  if (params.buffer) {
    RepetitionTester Testers[BranchPattern_Count][ArrayCount(TestFunctions)] =
        {};
    for (;;) {
      for (u32 Pattern = 0; Pattern < BranchPattern_Count; ++Pattern) {
        char const *PatternName = FillWithBranchPattern(
            (branch_pattern)Pattern, params.bufferSize, (u8 *)params.buffer);

        for (u32 FuncIndex = 0; FuncIndex < ArrayCount(TestFunctions);
             ++FuncIndex) {
          RepetitionTester Tester = Testers[Pattern][FuncIndex];
          test_function TestFunc = TestFunctions[FuncIndex];

          printf("\n--- %s, %s ---\n", TestFunc.Name, PatternName);
          Tester.newTestWave(params.bufferSize);

          REPETITION_TEST_BEGIN(Tester) {
            REPETITION_TEST_START_TIMING(Tester);
            TestFunc.Func(params.bufferSize, (u8 *)params.buffer);
            REPETITION_TEST_END_TIMING(Tester);
            REPETITION_TEST_COUNT_BYTES(Tester, params.bufferSize);
          }
        }
      }
    }
  } else {
    fprintf(stderr, "Unable to allocate memory buffer for testing");
  }

  handleDeallocation(&params, &params.buffer);
  return 0;
}
