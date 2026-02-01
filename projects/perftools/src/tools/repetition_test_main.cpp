#include "common/repetition_tester.hpp"
#include "common/test_helpers.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

// Test 1: fread (standard C library)
static void testFread(ReadParameters *params) {
  printf("\n=== Testing fread (AllocType: %s) ===\n",
         params->allocType == AllocType_Malloc      ? "Malloc"
         : params->allocType == AllocType_HugePages ? "HugePages"
                                                    : "None");

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
    size_t bytesRead = fread(buffer, 1, params->bufferSize, f);
    REPETITION_TEST_END_TIMING(tester);

    if (bytesRead != params->bufferSize) {
      fprintf(stderr, "ERROR: fread returned %zu bytes, expected %llu\n",
              bytesRead, (unsigned long long)params->bufferSize);
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
         params->allocType == AllocType_Malloc      ? "Malloc"
         : params->allocType == AllocType_HugePages ? "HugePages"
                                                    : "None");

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
    ssize_t bytesRead = read(fd, buffer, params->bufferSize);
    REPETITION_TEST_END_TIMING(tester);

    if (bytesRead != (ssize_t)params->bufferSize) {
      fprintf(stderr, "ERROR: read returned %zd bytes, expected %llu\n",
              bytesRead, (unsigned long long)params->bufferSize);
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
         params->allocType == AllocType_Malloc      ? "Malloc"
         : params->allocType == AllocType_HugePages ? "HugePages"
                                                    : "None");

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
    while (totalBytesRead < params->bufferSize) {
      u64 toRead = (params->bufferSize - totalBytesRead < CHUNK_SIZE)
                       ? (params->bufferSize - totalBytesRead)
                       : CHUNK_SIZE;
      size_t bytesRead = fread(buffer, 1, toRead, f);
      if (bytesRead == 0)
        break;
      totalBytesRead += bytesRead;
    }
    REPETITION_TEST_END_TIMING(tester);

    if (totalBytesRead != params->bufferSize) {
      fprintf(stderr,
              "ERROR: fread chunked returned %llu bytes, expected %llu\n",
              (unsigned long long)totalBytesRead,
              (unsigned long long)params->bufferSize);
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
         params->allocType == AllocType_Malloc      ? "Malloc"
         : params->allocType == AllocType_HugePages ? "HugePages"
                                                    : "None");

  RepetitionTester tester;
  tester.newTestWave(params->fileSize, params->testTimeMs);

  REPETITION_TEST_BEGIN(tester) {
    char *buffer = params->buffer;
    handleAllocation(params, &buffer);

    if (!buffer) {
      break;
    }

    REPETITION_TEST_START_TIMING(tester);
    // with clang-18 x64, on -O1, this loops takes 13 bytes
    // CPU Frequency / Bandwith: Cycles per loop instance:
    // (2.7×10^9)÷(1.39×1024^3) = 1.81 cycles
    for (u64 index = 0; index < params->bufferSize; ++index) {
      buffer[index] = (char)index;
    }
    REPETITION_TEST_END_TIMING(tester);

    REPETITION_TEST_COUNT_BYTES(tester, params->bufferSize);

    handleDeallocation(params, &buffer);
  }
}

// Test 5: Write to all bytes backward (reverse order write performance)
static void testWriteToAllBytesBackward(ReadParameters *params) {
  printf("\n=== Testing write to all bytes backward (AllocType: %s) ===\n",
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
    for (i64 index = (i64)params->bufferSize - 1; index >= 0; --index) {
      buffer[index] = (char)(index & 0xFF);
    }
    REPETITION_TEST_END_TIMING(tester);

    REPETITION_TEST_COUNT_BYTES(tester, params->bufferSize);

    handleDeallocation(params, &buffer);
  }
}

// Test 6: Memory map file and touch every page
static void testMmapAndTouchPages(ReadParameters *params) {
  printf("\n=== Testing mmap file and touch every page ===\n");

  RepetitionTester tester;
  tester.newTestWave(params->bufferSize, params->testTimeMs);

  const u64 PAGE_SIZE = 4096; // Standard 4KB page size

  REPETITION_TEST_BEGIN(tester) {
    int fd = open(params->filename, O_RDONLY);
    if (fd < 0) {
      fprintf(stderr, "ERROR: Could not open %s\n", params->filename);
      break;
    }

    REPETITION_TEST_START_TIMING(tester);

    // Memory map the file
    char *mapped = (char *)mmap(nullptr, params->bufferSize, PROT_READ,
                                MAP_PRIVATE, fd, 0);

    if (mapped == MAP_FAILED) {
      fprintf(stderr, "ERROR: Could not mmap file\n");
      close(fd);
      tester.testMode = RepetitionTester::Error;
      break;
    }

    // Touch every page to force page faults
    volatile char touchResult = 0;
    for (u64 offset = 0; offset < params->bufferSize; offset += PAGE_SIZE) {
      touchResult += mapped[offset];
    }
    // Touch the last byte if file size is not page-aligned
    if (params->bufferSize % PAGE_SIZE != 0) {
      touchResult += mapped[params->bufferSize - 1];
    }

    REPETITION_TEST_END_TIMING(tester);

    REPETITION_TEST_COUNT_BYTES(tester, params->bufferSize);

    // Clean up
    munmap(mapped, params->bufferSize);
    close(fd);
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

  AllocationType allocTypes[3] = {AllocType_None, AllocType_Malloc,
                                  AllocType_HugePages};

  // Array of test functions
  TestFunction tests[] = {
      // testFread, testRead, testFreadChunked,
      testWriteToAllBytes,
      // testWriteToAllBytesBackward,
      // testMmapAndTouchPages
  };
  const int testsCount = 1;

  while (true) {

    // Run each test with all allocation types
    for (int testIdx = 0; testIdx < testsCount; ++testIdx) {
      for (int typeIdx = 0; typeIdx < 3; ++typeIdx) {
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

        ReadParameters params = {
            {fileSize, buffer, allocType, testTimeMs, nullptr, 0}, filename};

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
