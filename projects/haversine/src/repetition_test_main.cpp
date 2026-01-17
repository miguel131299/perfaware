#include "haversine/repetition_tester.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Get file size
static u64 getFileSize(const char* filename) {
  struct stat st;
  if (stat(filename, &st) != 0) {
    fprintf(stderr, "ERROR: Could not stat file %s\n", filename);
    return 0;
  }
  return st.st_size;
}

// Test 1: fread (standard C library)
static void testFread(const char* filename, u64 fileSize) {
  printf("\n=== Testing fread ===\n");
  
  RepetitionTester tester;
  tester.newTestWave(fileSize, 10000);  // Try for 10 seconds
  
  REPETITION_TEST_BEGIN(tester) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
      fprintf(stderr, "ERROR: Could not open %s\n", filename);
      break;
    }
    
    // Allocate buffer for reading
    char* buffer = (char*)malloc(fileSize);
    if (!buffer) {
      fprintf(stderr, "ERROR: Could not allocate buffer\n");
      fclose(f);
      break;
    }
    
    REPETITION_TEST_START_TIMING(tester);
    size_t bytesRead = fread(buffer, 1, fileSize, f);
    REPETITION_TEST_END_TIMING(tester);
    
    if (bytesRead != fileSize) {
      fprintf(stderr, "ERROR: fread returned %zu bytes, expected %llu\n", bytesRead, (unsigned long long)fileSize);
      tester.testMode = RepetitionTester::Error;
    }
    
    REPETITION_TEST_COUNT_BYTES(tester, bytesRead);
    
    free(buffer);
    fclose(f);
  }
}

// Test 2: read (POSIX syscall)
static void testRead(const char* filename, u64 fileSize) {
  printf("\n=== Testing read (POSIX syscall) ===\n");
  
  RepetitionTester tester;
  tester.newTestWave(fileSize, 10000);  // Try for 10 seconds
  
  REPETITION_TEST_BEGIN(tester) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
      fprintf(stderr, "ERROR: Could not open %s\n", filename);
      break;
    }
    
    // Allocate buffer for reading
    char* buffer = (char*)malloc(fileSize);
    if (!buffer) {
      fprintf(stderr, "ERROR: Could not allocate buffer\n");
      close(fd);
      break;
    }
    
    REPETITION_TEST_START_TIMING(tester);
    ssize_t bytesRead = read(fd, buffer, fileSize);
    REPETITION_TEST_END_TIMING(tester);
    
    if (bytesRead != (ssize_t)fileSize) {
      fprintf(stderr, "ERROR: read returned %zd bytes, expected %llu\n", bytesRead, (unsigned long long)fileSize);
      tester.testMode = RepetitionTester::Error;
    }
    
    REPETITION_TEST_COUNT_BYTES(tester, (u64)bytesRead);
    
    free(buffer);
    close(fd);
  }
}

// Test 3: fread with smaller buffer (simulating chunked read)
static void testFreadChunked(const char* filename, u64 fileSize) {
  printf("\n=== Testing fread (64KB chunks) ===\n");
  
  RepetitionTester tester;
  tester.newTestWave(fileSize, 10000);  // Try for 10 seconds
  
  const u64 CHUNK_SIZE = 64 * 1024;  // 64KB chunks
  
  REPETITION_TEST_BEGIN(tester) {
    FILE* f = fopen(filename, "rb");
    if (!f) {
      fprintf(stderr, "ERROR: Could not open %s\n", filename);
      break;
    }
    
    char* buffer = (char*)malloc(CHUNK_SIZE);
    if (!buffer) {
      fprintf(stderr, "ERROR: Could not allocate buffer\n");
      fclose(f);
      break;
    }
    
    u64 totalBytesRead = 0;
    
    REPETITION_TEST_START_TIMING(tester);
    while (totalBytesRead < fileSize) {
      u64 toRead = (fileSize - totalBytesRead < CHUNK_SIZE) ? (fileSize - totalBytesRead) : CHUNK_SIZE;
      size_t bytesRead = fread(buffer, 1, toRead, f);
      if (bytesRead == 0) break;
      totalBytesRead += bytesRead;
    }
    REPETITION_TEST_END_TIMING(tester);
    
    if (totalBytesRead != fileSize) {
      fprintf(stderr, "ERROR: fread chunked returned %llu bytes, expected %llu\n", 
              (unsigned long long)totalBytesRead, (unsigned long long)fileSize);
      tester.testMode = RepetitionTester::Error;
    }
    
    REPETITION_TEST_COUNT_BYTES(tester, totalBytesRead);
    
    free(buffer);
    fclose(f);
  }
}

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
    fprintf(stderr, "  Example: %s data_json_1000000.json\n", argv[0]);
    return 1;
  }
  
  const char* filename = argv[1];
  u64 fileSize = getFileSize(filename);
  
  if (fileSize == 0) {
    fprintf(stderr, "ERROR: File not found or is empty: %s\n", filename);
    return 1;
  }
  
  printf("Testing file: %s (%llu bytes)\n", filename, (unsigned long long)fileSize);
  
  // Run all tests
  testFread(filename, fileSize);
  testRead(filename, fileSize);
  testFreadChunked(filename, fileSize);
  
  printf("\nAll tests completed.\n");
  return 0;
}
