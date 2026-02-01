#include "common/platform_metrics.hpp"
#include "common/types.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// Constants for the test
const u64 PAGE_SIZE = 4096;
const u64 PAGE_COUNT = 4096; // 4096 pages × 4KB = 16MB
const u64 BUFFER_SIZE = PAGE_SIZE * PAGE_COUNT;

// Allocate memory using OS primitives
static void *allocateBuffer() {
#ifdef _WIN32
  void *buffer = VirtualAlloc(nullptr, BUFFER_SIZE, MEM_RESERVE | MEM_COMMIT,
                              PAGE_READWRITE);
  if (!buffer) {
    fprintf(stderr, "ERROR: VirtualAlloc failed\n");
    return nullptr;
  }
#else
  void *buffer = mmap(nullptr, BUFFER_SIZE, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (buffer == MAP_FAILED) {
    fprintf(stderr, "ERROR: mmap failed\n");
    return nullptr;
  }
#endif
  return buffer;
}

// Free memory using OS primitives
static void freeBuffer(void *buffer) {
  if (!buffer)
    return;

#ifdef _WIN32
  VirtualFree(buffer, 0, MEM_RELEASE);
#else
  munmap(buffer, BUFFER_SIZE);
#endif
}

// Touch N pages sequentially (0-indexed pages)
static void touchPages(void *buffer, u64 pageCount) {
  volatile char *ptr = (volatile char *)buffer;
  for (u64 i = 0; i < pageCount; ++i) {
    ptr[i * PAGE_SIZE] = (char)i; // Touch first byte of each page
  }
}

// Touch N pages in reverse order (from end to beginning)
static void touchPagesBackward(void *buffer, u64 pageCount) {
  volatile char *ptr = (volatile char *)buffer;
  for (i64 i = (i64)pageCount - 1; i >= 0; --i) {
    ptr[i * PAGE_SIZE] = (char)i; // Touch first byte of each page, backwards
  }
}

// Run a single trial: touch pageCount pages and measure page faults
static u64 runTrial(void *buffer, u64 pageCount) {
  InitPageFaultTracking();

  u64 faultsBefore = ReadPageFaultCount();
  touchPages(buffer, pageCount);
  u64 faultsAfter = ReadPageFaultCount();

  u64 pageFaultsDuring = faultsAfter - faultsBefore;
  return pageFaultsDuring;
}

// Run a single trial touching pages backward
static u64 runTrialBackward(void *buffer, u64 pageCount) {
  InitPageFaultTracking();

  u64 faultsBefore = ReadPageFaultCount();
  touchPagesBackward(buffer, pageCount);
  u64 faultsAfter = ReadPageFaultCount();

  u64 pageFaultsDuring = faultsAfter - faultsBefore;
  return pageFaultsDuring;
}

// Run a ramp test: touch 0, 1, 2, ..., maxPages and measure faults for each
static void runRampTest(u64 maxPages) {
  printf("=== Page Fault Ramp Test ===\n");
  printf("Buffer size: %llu bytes (%llu pages × %llu KB)\n",
         (unsigned long long)BUFFER_SIZE, (unsigned long long)PAGE_COUNT,
         (unsigned long long)(PAGE_SIZE / 1024));
  printf("\nPage Count, Touch Count, Fault Count, Extra Faults\n");

  for (u64 pageCount = 0; pageCount <= maxPages; ++pageCount) {
    void *buffer = allocateBuffer();
    if (!buffer) {
      fprintf(stderr, "ERROR: Failed to allocate buffer\n");
      return;
    }

    u64 faults = runTrial(buffer, pageCount);
    i64 extraFaults = (i64)faults - (i64)pageCount;

    printf("%llu, %llu, %llu, %lld\n", (unsigned long long)PAGE_COUNT,
           (unsigned long long)pageCount, (unsigned long long)faults,
           (long long)extraFaults);

    freeBuffer(buffer);
  }
  printf("\n");
}

// Run a ramp test backward: touch pages from end to beginning
static void runRampTestBackward(u64 maxPages) {
  printf("=== Page Fault Ramp Test (Backward) ===\n");
  printf("Buffer size: %llu bytes (%llu pages × %llu KB)\n",
         (unsigned long long)BUFFER_SIZE, (unsigned long long)PAGE_COUNT,
         (unsigned long long)(PAGE_SIZE / 1024));
  printf("\nPage Count, Touch Count, Fault Count, Extra Faults\n");

  for (u64 pageCount = 0; pageCount <= maxPages; ++pageCount) {
    void *buffer = allocateBuffer();
    if (!buffer) {
      fprintf(stderr, "ERROR: Failed to allocate buffer\n");
      return;
    }

    u64 faults = runTrialBackward(buffer, pageCount);
    i64 extraFaults = (i64)faults - (i64)pageCount;

    printf("%llu, %llu, %llu, %lld\n", (unsigned long long)PAGE_COUNT,
           (unsigned long long)pageCount, (unsigned long long)faults,
           (long long)extraFaults);

    freeBuffer(buffer);
  }
  printf("\n");
}

int main(int argc, char **argv) {
  u64 maxPages = 64;                 // Default: test up to 64 pages
  const char *direction = "forward"; // Default: forward direction

  if (argc >= 2) {
    maxPages = std::strtoull(argv[1], nullptr, 10);
    if (maxPages == 0 || maxPages > PAGE_COUNT) {
      fprintf(stderr, "ERROR: maxPages must be between 1 and %llu\n",
              (unsigned long long)PAGE_COUNT);
      return 1;
    }
  }

  if (argc >= 3) {
    direction = argv[2];
    if (strcmp(direction, "forward") != 0 &&
        strcmp(direction, "backward") != 0 && strcmp(direction, "both") != 0) {
      fprintf(stderr,
              "ERROR: direction must be 'forward', 'backward', or 'both'\n");
      return 1;
    }
  }

  printf("Page Fault Testing Tool\n");
  printf("========================\n\n");

  if (strcmp(direction, "forward") == 0 || strcmp(direction, "both") == 0) {
    runRampTest(maxPages);
  }

  if (strcmp(direction, "backward") == 0 || strcmp(direction, "both") == 0) {
    runRampTestBackward(maxPages);
  }

  printf("Test completed.\n");
  return 0;
}
