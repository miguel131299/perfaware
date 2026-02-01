#include "common/repetition_tester.hpp"
#include "common/test_helpers.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

// Assembly function declarations (Linux x64 System V ABI)
extern "C" {
void MOVAllBytesASM(u64 count, u8 *data);
void NOPAllBytesASM(u64 count);
void CMPAllBytesASM(u64 count);
void DECAllBytesASM(u64 count);
}

// Test: Write to all bytes (C++ version)
static void testWriteToAllBytes(TestParameters *params) {
  printf("\n=== Testing WriteToAllBytes (C++, AllocType: %s) ===\n",
         params->allocType == AllocType_Malloc      ? "Malloc"
         : params->allocType == AllocType_HugePages ? "HugePages"
                                                    : "None");

  RepetitionTester tester;
  tester.newTestWave(params->bufferSize, params->testTimeMs);

  REPETITION_TEST_BEGIN(tester) {
    char *buffer = params->buffer;
    handleAllocation(params, &buffer);

    if (!buffer) {
      break;
    }

    REPETITION_TEST_START_TIMING(tester);
    for (u64 index = 0; index < params->bufferSize; ++index) {
      buffer[index] = (char)index;
    }
    REPETITION_TEST_END_TIMING(tester);

    REPETITION_TEST_COUNT_BYTES(tester, params->bufferSize);

    handleDeallocation(params, &buffer);
  }
}

// Test: MOV instruction (ASM version)
static void testMOVAllBytes(TestParameters *params) {
  printf("\n=== Testing MOVAllBytesASM (AllocType: %s) ===\n",
         params->allocType == AllocType_Malloc      ? "Malloc"
         : params->allocType == AllocType_HugePages ? "HugePages"
                                                    : "None");

  RepetitionTester tester;
  tester.newTestWave(params->bufferSize, params->testTimeMs);

  REPETITION_TEST_BEGIN(tester) {
    char *buffer = params->buffer;
    handleAllocation(params, &buffer);

    if (!buffer) {
      break;
    }

    REPETITION_TEST_START_TIMING(tester);
    MOVAllBytesASM(params->bufferSize, (u8 *)buffer);
    REPETITION_TEST_END_TIMING(tester);

    REPETITION_TEST_COUNT_BYTES(tester, params->bufferSize);

    handleDeallocation(params, &buffer);
  }
}

// Test: NOP instructions
static void testNOPAllBytes(TestParameters *params) {
  printf("\n=== Testing NOPAllBytesASM (AllocType: %s) ===\n",
         params->allocType == AllocType_Malloc      ? "Malloc"
         : params->allocType == AllocType_HugePages ? "HugePages"
                                                    : "None");

  RepetitionTester tester;
  tester.newTestWave(params->bufferSize, params->testTimeMs);

  REPETITION_TEST_BEGIN(tester) {
    char *buffer = params->buffer;
    handleAllocation(params, &buffer);

    if (!buffer) {
      break;
    }

    REPETITION_TEST_START_TIMING(tester);
    NOPAllBytesASM(params->bufferSize);
    REPETITION_TEST_END_TIMING(tester);

    REPETITION_TEST_COUNT_BYTES(tester, params->bufferSize);

    handleDeallocation(params, &buffer);
  }
}

// Test: CMP instructions
static void testCMPAllBytes(TestParameters *params) {
  printf("\n=== Testing CMPAllBytesASM (AllocType: %s) ===\n",
         params->allocType == AllocType_Malloc      ? "Malloc"
         : params->allocType == AllocType_HugePages ? "HugePages"
                                                    : "None");

  RepetitionTester tester;
  tester.newTestWave(params->bufferSize, params->testTimeMs);

  REPETITION_TEST_BEGIN(tester) {
    char *buffer = params->buffer;
    handleAllocation(params, &buffer);

    if (!buffer) {
      break;
    }

    REPETITION_TEST_START_TIMING(tester);
    CMPAllBytesASM(params->bufferSize);
    REPETITION_TEST_END_TIMING(tester);

    REPETITION_TEST_COUNT_BYTES(tester, params->bufferSize);

    handleDeallocation(params, &buffer);
  }
}

// Test: DEC instructions
static void testDECAllBytes(TestParameters *params) {
  printf("\n=== Testing DECAllBytesASM (AllocType: %s) ===\n",
         params->allocType == AllocType_Malloc      ? "Malloc"
         : params->allocType == AllocType_HugePages ? "HugePages"
                                                    : "None");

  RepetitionTester tester;
  tester.newTestWave(params->bufferSize, params->testTimeMs);

  REPETITION_TEST_BEGIN(tester) {
    char *buffer = params->buffer;
    handleAllocation(params, &buffer);

    if (!buffer) {
      break;
    }

    REPETITION_TEST_START_TIMING(tester);
    DECAllBytesASM(params->bufferSize);
    REPETITION_TEST_END_TIMING(tester);

    REPETITION_TEST_COUNT_BYTES(tester, params->bufferSize);

    handleDeallocation(params, &buffer);
  }
}

// Function pointer type for test functions
typedef void (*TestFunction)(TestParameters *);

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename> [test_time_ms]\n", argv[0]);
    fprintf(stderr, "  filename: file to use for buffer size determination\n");
    fprintf(stderr, "  test_time_ms: duration for each test in milliseconds "
                    "(default: 10000)\n");
    fprintf(stderr, "  Example: %s data.json\n", argv[0]);
    fprintf(stderr, "  Example: %s data.json 5000\n", argv[0]);
    return 1;
  }

  const char *filename = argv[1];

  struct stat st;
  if (stat(filename, &st) != 0) {
    fprintf(stderr, "ERROR: Could not stat file %s\n", filename);
    return 1;
  }

  u64 bufferSize = st.st_size;
  if (bufferSize == 0) {
    fprintf(stderr, "ERROR: File size must be non-zero\n");
    return 1;
  }

  u64 testTimeMs = 10000; // Default: 10 seconds

  if (argc >= 3) {
    testTimeMs = std::strtoull(argv[2], nullptr, 10);
    if (testTimeMs == 0) {
      fprintf(stderr, "ERROR: Invalid test time (must be > 0)\n");
      return 1;
    }
  }

  printf("Testing with file: %s (%llu bytes, %.2f MB)\n", filename,
         (unsigned long long)bufferSize, bufferSize / (1024.0 * 1024.0));
  printf("========================================\n\n");

  // Array of test functions
  TestFunction tests[] = {testWriteToAllBytes, testMOVAllBytes, testNOPAllBytes,
                          testCMPAllBytes, testDECAllBytes};
  const int testsCount = 5;

  // Allocate buffer once (reused across all tests)
  char *buffer = (char *)malloc(bufferSize);
  if (!buffer) {
    fprintf(stderr, "ERROR: Could not allocate buffer\n");
    return 1;
  }

  // Run each test with AllocType_None
  for (int testIdx = 0; testIdx < testsCount; ++testIdx) {
    TestParameters params = {bufferSize, buffer,  AllocType_None,
                             testTimeMs, nullptr, 0};

    tests[testIdx](&params);
    printf("\n");
  }

  free(buffer);

  printf("All tests completed.\n");
  return 0;
}
