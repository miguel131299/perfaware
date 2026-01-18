#include "haversine/repetition_tester.hpp"
#include "haversine/types.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

enum AllocationType {
  AllocType_None,
  AllocType_Malloc,
  AllocType_Count,
};

struct ReadParameters {
  const char *filename;
  u64 fileSize;
  char *buffer; // Pre-allocated buffer
  AllocationType allocType;
  u64 testTimeMs; // Test duration in milliseconds
};

// Handle memory allocation based on allocation type
static void handleAllocation(ReadParameters *params, char **buffer) {
  switch (params->allocType) {
  case AllocType_None: {
    // Use pre-allocated buffer, don't allocate
  } break;

  case AllocType_Malloc: {
    *buffer = (char *)malloc(params->fileSize);
    if (!*buffer) {
      fprintf(stderr, "ERROR: Could not allocate buffer\n");
    }
  } break;

  default: {
    fprintf(stderr, "ERROR: Unrecognized allocation type\n");
  } break;
  }
}

// Handle memory deallocation
static void handleDeallocation(ReadParameters *params, char **buffer) {
  switch (params->allocType) {
  case AllocType_None: {
    // Nothing to deallocate
  } break;

  case AllocType_Malloc: {
    if (*buffer) {
      free(*buffer);
      *buffer = nullptr;
    }
  } break;

  default: {
    fprintf(stderr, "ERROR: Unrecognized allocation type\n");
  } break;
  }
}

// Test 1: fread (standard C library)
static void testFread(ReadParameters *params) {
  printf("\n=== Testing fread (AllocType: %s) ===\n",
         params->allocType == AllocType_Malloc ? "Malloc" : "None");

  RepetitionTester tester;
  tester.newTestWave(params->fileSize, params->testTimeMs);

  REPETITION_TEST_BEGIN(tester) {
    FILE *f = fopen(params->filename, "rb");
    if (!f) {
      fprintf(stderr, "ERROR: Could not open %s\n", params->filename);
      break;
    }

    char *buffer = params->buffer;
    handleAllocation(params, &buffer);

    if (!buffer) {
      fclose(f);
      break;
    }

    REPETITION_TEST_START_TIMING(tester);
    size_t bytesRead = fread(buffer, 1, params->fileSize, f);
    REPETITION_TEST_END_TIMING(tester);

    if (bytesRead != params->fileSize) {
      fprintf(stderr, "ERROR: fread returned %zu bytes, expected %llu\n",
              bytesRead, (unsigned long long)params->fileSize);
      tester.testMode = RepetitionTester::Error;
    }

    REPETITION_TEST_COUNT_BYTES(tester, bytesRead);

    handleDeallocation(params, &buffer);
    fclose(f);
  }
}

// Test 2: read (POSIX syscall)
static void testRead(ReadParameters *params) {
  printf("\n=== Testing read (POSIX syscall, AllocType: %s) ===\n",
         params->allocType == AllocType_Malloc ? "Malloc" : "None");

  RepetitionTester tester;
  tester.newTestWave(params->fileSize, params->testTimeMs);

  REPETITION_TEST_BEGIN(tester) {
    int fd = open(params->filename, O_RDONLY);
    if (fd < 0) {
      fprintf(stderr, "ERROR: Could not open %s\n", params->filename);
      break;
    }

    char *buffer = params->buffer;
    handleAllocation(params, &buffer);

    if (!buffer) {
      close(fd);
      break;
    }

    REPETITION_TEST_START_TIMING(tester);
    ssize_t bytesRead = read(fd, buffer, params->fileSize);
    REPETITION_TEST_END_TIMING(tester);

    if (bytesRead != (ssize_t)params->fileSize) {
      fprintf(stderr, "ERROR: read returned %zd bytes, expected %llu\n",
              bytesRead, (unsigned long long)params->fileSize);
      tester.testMode = RepetitionTester::Error;
    }

    REPETITION_TEST_COUNT_BYTES(tester, (u64)bytesRead);

    handleDeallocation(params, &buffer);
    close(fd);
  }
}

// Test 3: fread with smaller buffer (simulating chunked read)
static void testFreadChunked(ReadParameters *params) {
  printf("\n=== Testing fread (64KB chunks, AllocType: %s) ===\n",
         params->allocType == AllocType_Malloc ? "Malloc" : "None");

  RepetitionTester tester;
  tester.newTestWave(params->fileSize, params->testTimeMs);

  const u64 CHUNK_SIZE = 64 * 1024; // 64KB chunks

  REPETITION_TEST_BEGIN(tester) {
    FILE *f = fopen(params->filename, "rb");
    if (!f) {
      fprintf(stderr, "ERROR: Could not open %s\n", params->filename);
      break;
    }

    char *buffer = params->buffer;
    handleAllocation(params, &buffer);

    if (!buffer) {
      fclose(f);
      break;
    }

    u64 totalBytesRead = 0;

    REPETITION_TEST_START_TIMING(tester);
    while (totalBytesRead < params->fileSize) {
      u64 toRead = (params->fileSize - totalBytesRead < CHUNK_SIZE)
                       ? (params->fileSize - totalBytesRead)
                       : CHUNK_SIZE;
      size_t bytesRead = fread(buffer, 1, toRead, f);
      if (bytesRead == 0)
        break;
      totalBytesRead += bytesRead;
    }
    REPETITION_TEST_END_TIMING(tester);

    if (totalBytesRead != params->fileSize) {
      fprintf(stderr,
              "ERROR: fread chunked returned %llu bytes, expected %llu\n",
              (unsigned long long)totalBytesRead,
              (unsigned long long)params->fileSize);
      tester.testMode = RepetitionTester::Error;
    }

    REPETITION_TEST_COUNT_BYTES(tester, totalBytesRead);

    handleDeallocation(params, &buffer);
    fclose(f);
  }
}

// Test 4: Write to all bytes (memory write performance, first-touch behavior)
static void testWriteToAllBytes(ReadParameters *params) {
  printf("\n=== Testing write to all bytes (AllocType: %s) ===\n",
         params->allocType == AllocType_Malloc ? "Malloc" : "None");

  RepetitionTester tester;
  tester.newTestWave(params->fileSize, params->testTimeMs);

  REPETITION_TEST_BEGIN(tester) {
    char *buffer = params->buffer;
    handleAllocation(params, &buffer);

    if (!buffer) {
      break;
    }

    REPETITION_TEST_START_TIMING(tester);
    for (u64 index = 0; index < params->fileSize; ++index) {
      buffer[index] = (char)(index & 0xFF);
    }
    REPETITION_TEST_END_TIMING(tester);

    REPETITION_TEST_COUNT_BYTES(tester, params->fileSize);

    handleDeallocation(params, &buffer);
  }
}

// Test 5: Write to all bytes backward (reverse order write performance)
static void testWriteToAllBytesBackward(ReadParameters *params) {
  printf("\n=== Testing write to all bytes backward (AllocType: %s) ===\n",
         params->allocType == AllocType_Malloc ? "Malloc" : "None");

  RepetitionTester tester;
  tester.newTestWave(params->fileSize, params->testTimeMs);

  REPETITION_TEST_BEGIN(tester) {
    char *buffer = params->buffer;
    handleAllocation(params, &buffer);

    if (!buffer) {
      break;
    }

    REPETITION_TEST_START_TIMING(tester);
    for (i64 index = (i64)params->fileSize - 1; index >= 0; --index) {
      buffer[index] = (char)(index & 0xFF);
    }
    REPETITION_TEST_END_TIMING(tester);

    REPETITION_TEST_COUNT_BYTES(tester, params->fileSize);

    handleDeallocation(params, &buffer);
  }
}

// Function pointer type for test functions
typedef void (*TestFunction)(ReadParameters *);

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename> [test_time_ms]\n", argv[0]);
    fprintf(stderr, "  test_time_ms: duration for each test in milliseconds "
                    "(default: 10000)\n");
    fprintf(stderr, "  Example: %s data_json_1000000.json\n", argv[0]);
    fprintf(stderr, "  Example: %s data_json_1000000.json 5000\n", argv[0]);
    return 1;
  }

  const char *filename = argv[1];
  u64 testTimeMs = 10000; // Default: 10 seconds

  if (argc >= 3) {
    testTimeMs = std::strtoull(argv[2], nullptr, 10);
    if (testTimeMs == 0) {
      fprintf(stderr, "ERROR: Invalid test time (must be > 0)\n");
      return 1;
    }
  }

  struct stat st;
  if (stat(filename, &st) != 0) {
    fprintf(stderr, "ERROR: Could not stat file %s\n", filename);
    return 1;
  }

  u64 fileSize = st.st_size;
  if (fileSize == 0) {
    fprintf(stderr, "ERROR: File not found or is empty: %s\n", filename);
    return 1;
  }

  printf("Testing file: %s (%llu bytes)\n", filename,
         (unsigned long long)fileSize);
  printf("========================================\n\n");

  AllocationType allocTypes[2] = {AllocType_None, AllocType_Malloc};

  // Array of test functions
  TestFunction tests[] = {testWriteToAllBytes, testWriteToAllBytesBackward};
  const int testsCount = 2;

  while (true) {

    // Run each test with both allocation types
    for (int testIdx = 0; testIdx < testsCount; ++testIdx) {
      for (int typeIdx = 0; typeIdx < 2; ++typeIdx) {
        AllocationType allocType = allocTypes[typeIdx];

        // Allocate buffer once for AllocType_None (reused across tests)
        char *buffer = nullptr;
        if (allocType == AllocType_None) {
          buffer = (char *)malloc(fileSize);
          if (!buffer) {
            fprintf(stderr,
                    "ERROR: Could not allocate buffer for AllocType_None\n");
            return 1;
          }
        }

        ReadParameters params = {filename, fileSize, buffer, allocType,
                                 testTimeMs};

        // Run the test
        tests[testIdx](&params);

        // Clean up buffer for this allocation type
        if (allocType == AllocType_None && buffer) {
          free(buffer);
        }

        printf("\n");
      }
    }
  }

  printf("All tests completed.\n");
  return 0;
}
