#include "haversine/platform_metrics.hpp"
#include "haversine/pointer_decompose.hpp"
#include "haversine/types.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

const u64 PAGE_SIZE = 4096;

// Allocate buffer
static void *allocateBuffer(u64 size) {
#ifdef _WIN32
  void *buffer =
      VirtualAlloc(nullptr, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  if (!buffer) {
    fprintf(stderr, "ERROR: VirtualAlloc failed\n");
    return nullptr;
  }
#else
  void *buffer = mmap(nullptr, size, PROT_READ | PROT_WRITE,
                      MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (buffer == MAP_FAILED) {
    fprintf(stderr, "ERROR: mmap failed\n");
    return nullptr;
  }
#endif
  return buffer;
}

// Free buffer
static void freeBuffer(void *buffer, u64 size) {
  if (!buffer)
    return;

#ifdef _WIN32
  VirtualFree(buffer, 0, MEM_RELEASE);
#else
  munmap(buffer, size);
#endif
}

// Touch N pages and count faults
static u64 touchPagesAndCountFaults(void *buffer, u64 pageCount) {
  InitPageFaultTracking();

  u64 faultsBefore = ReadPageFaultCount();

  volatile char *ptr = (volatile char *)buffer;
  for (u64 i = 0; i < pageCount; ++i) {
    ptr[i * PAGE_SIZE] = (char)i;
  }

  u64 faultsAfter = ReadPageFaultCount();
  return faultsAfter - faultsBefore;
}

// Print address decomposition for a pointer
static void printPointerInfo(const char *label, u64 addr) {
  printf("%s: 0x%016llx\n", label, (unsigned long long)addr);
  printf("  | PML4 | PDPT |   PD |   PT | Offset\n");
  PageDecompose d = decompose4K(addr);
  printf("  | %4u | %4u | %4u | %4u | %13u\n", d.pml4, d.pdpt, d.pd, d.pt,
         d.offset);
}

// Analyze page table transitions
struct PageTableStats {
  u64 uniquePDs;
  u64 uniquePTs;
};

// Result entry for accumulating without I/O
struct FaultResult {
  u64 pageCount;
  u64 faults;
  i64 extraFaults;
  PageDecompose prevDecomp;
  PageDecompose currDecomp;
};

static PageTableStats analyzePageTableTransitions(void *buffer, u64 pageCount) {
  PageTableStats stats = {0};

  if (pageCount == 0)
    return stats;

  volatile char *ptr = (volatile char *)buffer;

  u64 prevPD = 0;
  u64 prevPT = 0;
  bool first = true;

  for (u64 i = 0; i < pageCount; ++i) {
    u64 addr = (u64)&ptr[i * PAGE_SIZE];
    PageDecompose d = decompose4K(addr);

    if (first) {
      prevPD = d.pd;
      prevPT = d.pt;
      stats.uniquePDs = 1;
      stats.uniquePTs = 1;
      first = false;
    } else {
      if (d.pd != prevPD) {
        stats.uniquePDs++;
        prevPD = d.pd;
      }
      if (d.pt != prevPT) {
        stats.uniquePTs++;
        prevPT = d.pt;
      }
    }
  }

  return stats;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <pageCount> [bufferSizeMB]\n", argv[0]);
    fprintf(stderr, "       %s ramp [maxPageCount] [bufferSizeMB]\n", argv[0]);
    fprintf(stderr, "  Example: %s 10          # Touch 10 pages\n", argv[0]);
    fprintf(stderr,
            "  Example: %s 10 128      # Touch 10 pages from 128MB buffer\n",
            argv[0]);
    fprintf(stderr, "  Example: %s ramp 1000   # Ramp test up to 1000 pages\n",
            argv[0]);
    return 1;
  }

  // Check for ramp mode
  if (strcmp(argv[1], "ramp") == 0) {
    u64 maxPageCount = 2048;
    u64 bufferSizeMB = 256;

    if (argc >= 3) {
      maxPageCount = std::strtoull(argv[2], nullptr, 10);
    }
    if (argc >= 4) {
      bufferSizeMB = std::strtoull(argv[3], nullptr, 10);
    }

    u64 bufferSize = bufferSizeMB * 1024 * 1024;

    printf("=========== Ramp Page Fault Test ===========\n\n");

    // Print buffer base (allocate once just for reference)
    void *refBuffer = allocateBuffer(bufferSize);
    printf("Buffer base: ");
    PageDecompose baseD = decompose4K((u64)refBuffer);
    printf("| %u|%u|%u|%u|%u|\n\n", baseD.pml4, baseD.pdpt, baseD.pd, baseD.pt,
           baseD.offset);
    freeBuffer(refBuffer, bufferSize);

    // Preallocate results array to avoid I/O during measurement
    const u64 BATCH_SIZE = 1024;
    FaultResult *results = (FaultResult *)malloc(sizeof(FaultResult) * BATCH_SIZE);
    if (!results) {
      fprintf(stderr, "ERROR: Could not allocate results array\n");
      return 1;
    }

    u64 resultIdx = 0;
    i64 prevExtraFaults = 0;
    u64 lastIncreasePageCount = 0;
    PageDecompose prevDecomp = {0, 0, 0, 0, 0};

    // Silent measurement loop - no I/O
    for (u64 pageCount = 0; pageCount <= maxPageCount; ++pageCount) {
      // Allocate FRESH buffer for each iteration
      void *buffer = allocateBuffer(bufferSize);
      if (!buffer) {
        fprintf(stderr, "ERROR: Failed to allocate buffer\n");
        return 1;
      }

      u64 faults = touchPagesAndCountFaults(buffer, pageCount);
      i64 extraFaults = (i64)faults - (i64)pageCount;

      if (extraFaults > prevExtraFaults) {
        // Extra faults increased - record this
        FaultResult *res = &results[resultIdx];
        res->pageCount = pageCount;
        res->faults = faults;
        res->extraFaults = extraFaults;
        res->prevDecomp = prevDecomp;

        volatile char *ptr = (volatile char *)buffer;
        u64 currAddr = (u64)&ptr[pageCount * PAGE_SIZE];
        res->currDecomp = decompose4K(currAddr);

        prevDecomp = res->currDecomp;
        prevExtraFaults = extraFaults;
        lastIncreasePageCount = pageCount;
        resultIdx++;

        // If array is full, print batch and reset
        if (resultIdx >= BATCH_SIZE) {
          for (u64 i = 0; i < resultIdx; ++i) {
            FaultResult *r = &results[i];
            printf("Page %llu: %lld extra faults (%llu pages since last increase)\n",
                   (unsigned long long)r->pageCount,
                   (long long)r->extraFaults,
                   (i == 0) ? r->pageCount : r->pageCount - results[i - 1].pageCount);
            printf("    Previous Pointer: | %u|%u|%u|%u|%u|\n",
                   r->prevDecomp.pml4, r->prevDecomp.pdpt, r->prevDecomp.pd,
                   r->prevDecomp.pt, r->prevDecomp.offset);
            printf("    This Pointer:     | %u|%u|%u|%u|%u|\n",
                   r->currDecomp.pml4, r->currDecomp.pdpt, r->currDecomp.pd,
                   r->currDecomp.pt, r->currDecomp.offset);
            printf("\n");
          }
          resultIdx = 0;
        }
      }

      freeBuffer(buffer, bufferSize);
    }

    // Print remaining results
    for (u64 i = 0; i < resultIdx; ++i) {
      FaultResult *r = &results[i];
      printf("Page %llu: %lld extra faults (%llu pages since last increase)\n",
             (unsigned long long)r->pageCount,
             (long long)r->extraFaults,
             (i == 0) ? r->pageCount : r->pageCount - results[i - 1].pageCount);
      printf("    Previous Pointer: | %u|%u|%u|%u|%u|\n",
             r->prevDecomp.pml4, r->prevDecomp.pdpt, r->prevDecomp.pd,
             r->prevDecomp.pt, r->prevDecomp.offset);
      printf("    This Pointer:     | %u|%u|%u|%u|%u|\n",
             r->currDecomp.pml4, r->currDecomp.pdpt, r->currDecomp.pd,
             r->currDecomp.pt, r->currDecomp.offset);
      printf("\n");
    }

    free(results);

    printf("=========== Pattern Analysis ===========\n");
    printf("Extra faults increase at page transitions when:\n");
    printf("  - Page Table (PT) index changes\n");
    printf("  - Page Directory (PD) index changes\n");
    printf("  - New page table pages must be loaded\n");
    return 0;
  }

  // Normal mode: single page count analysis
  u64 pageCount = std::strtoull(argv[1], nullptr, 10);
  u64 bufferSizeMB = 256;

  if (argc >= 3) {
    bufferSizeMB = std::strtoull(argv[2], nullptr, 10);
  }

  u64 bufferSize = bufferSizeMB * 1024 * 1024;

  printf("=== Page Fault Analysis ===\n");
  printf("Buffer Size:  %llu MB (%llu bytes)\n",
         (unsigned long long)bufferSizeMB, (unsigned long long)bufferSize);
  printf("Pages to Touch: %llu\n\n", (unsigned long long)pageCount);

  void *buffer = allocateBuffer(bufferSize);
  if (!buffer) {
    return 1;
  }

  // Show pointer info
  printf("Buffer Address:\n");
  printPointerInfo("  Base", (u64)buffer);
  u64 endAddr = (u64)buffer + bufferSize - 1;
  printf("\nEnd Address:\n");
  printPointerInfo("  End", endAddr);

  // Touch pages and measure faults
  printf("\n--- Touching Pages ---\n");
  u64 faults = touchPagesAndCountFaults(buffer, pageCount);
  i64 extraFaults = (i64)faults - (i64)pageCount;

  printf("Pages Touched:    %llu\n", (unsigned long long)pageCount);
  printf("Page Faults:      %llu\n", (unsigned long long)faults);
  printf("Extra Faults:     %lld\n", (long long)extraFaults);

  // Analyze page table structure
  printf("\n--- Page Table Analysis ---\n");
  PageTableStats stats = analyzePageTableTransitions(buffer, pageCount);
  printf("Unique Page Directories (PD):  %llu\n",
         (unsigned long long)stats.uniquePDs);
  printf("Unique Page Tables (PT):       %llu\n",
         (unsigned long long)stats.uniquePTs);
  printf("\nTotal pages (PD + PT): %llu\n",
         (unsigned long long)(stats.uniquePTs + stats.uniquePDs));

  // Show page boundaries for sampled addresses
  printf("\n--- Sample Touched Addresses ---\n");
  volatile char *ptr = (volatile char *)buffer;

  // Print first 5 pages
  for (u64 i = 0; i < pageCount && i < 5; ++i) {
    u64 addr = (u64)&ptr[i * PAGE_SIZE];
    printf("\nPage %llu:\n", (unsigned long long)i);
    printPointerInfo("  Address", addr);
  }

  // Print gap indicator
  if (pageCount > 10) {
    printf("\n... (%llu pages in between) ...\n",
           (unsigned long long)(pageCount - 10));
  }

  // Print last 5 pages
  for (u64 i = (pageCount > 5) ? pageCount - 5 : 5; i < pageCount; ++i) {
    u64 addr = (u64)&ptr[i * PAGE_SIZE];
    printf("\nPage %llu:\n", (unsigned long long)i);
    printPointerInfo("  Address", addr);
  }

  freeBuffer(buffer, bufferSize);

  printf("\n");
  return 0;
}
