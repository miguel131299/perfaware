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
