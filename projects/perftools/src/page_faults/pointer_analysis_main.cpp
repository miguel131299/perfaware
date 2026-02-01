#include "common/pointer_decompose.hpp"
#include "common/types.hpp"

#include <cstdio>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

// Allocate memory using OS primitives
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

// Free memory using OS primitives
static void freeBuffer(void *buffer, u64 size) {
  if (!buffer)
    return;

#ifdef _WIN32
  VirtualFree(buffer, 0, MEM_RELEASE);
#else
  munmap(buffer, size);
#endif
}

// Analyze a single pointer
static void analyzePointer(const char *label, void *ptr) {
  u64 addr = (u64)ptr;
  printf("\n%s: 0x%016lx\n", label, addr);

  // Show 4K decomposition in table format
  printAddressTableHeader();
  printAddress(addr, PageSize::Size4K);
}

// Analyze multiple allocations
static void analyzeSequentialAllocations() {
  printf("=== Sequential Allocations (malloc) ===\n");
  printf("Purpose: Analyze how malloc allocates small 1KB blocks\n");
  printf("Observe: Whether allocations are within same page or different "
         "pages\n\n");

  for (int i = 0; i < 5; ++i) {
    void *ptr = malloc(1024);
    if (ptr) {
      analyzePointer("malloc(1024)", ptr);
      free(ptr);
    }
  }
}

// Analyze OS-level allocations
static void analyzeOSAllocations() {
  printf("\n=== OS-Level Allocations (mmap/VirtualAlloc) ===\n");
  printf("Purpose: Analyze OS-level allocations with 64KB chunks\n");
  printf("Observe: Page alignment and address space distribution\n\n");

  const u64 ALLOC_SIZE = 64 * 1024; // 64KB
  printf("Allocating %lu bytes per block...\n\n", ALLOC_SIZE);

  for (int i = 0; i < 3; ++i) {
    void *ptr = allocateBuffer(ALLOC_SIZE);
    if (ptr) {
      analyzePointer("OS allocation", ptr);
      freeBuffer(ptr, ALLOC_SIZE);
    }
  }
}

// Analyze pointer differences
static void analyzePointerDifferences() {
  printf("\n=== Pointer Differences within 1MB Buffer ===\n");
  printf("Purpose: Understand page boundaries within a single allocation\n");
  printf("Observe: How different offsets affect page table indices\n\n");

  const u64 BUFFER_SIZE = 1024 * 1024; // 1MB
  void *buffer = allocateBuffer(BUFFER_SIZE);

  if (!buffer) {
    fprintf(stderr, "ERROR: Could not allocate buffer\n");
    return;
  }

  u64 base_addr = (u64)buffer;
  printf("Base address: 0x%016lx\n", base_addr);
  printf("Buffer size: %lu bytes (%.2f MB)\n", BUFFER_SIZE,
         (f64)BUFFER_SIZE / (1024.0 * 1024.0));
  printf("Number of 4K pages: %lu\n", BUFFER_SIZE / 4096);
  printf("Number of 2M pages: %lu\n\n",
         (BUFFER_SIZE + (2 * 1024 * 1024) - 1) / (2 * 1024 * 1024));

  // Check addresses at different offsets
  u64 offsets[] = {0, 4096, 4096 * 2, 4096 * 512, 1024 * 1024 - 1};

  printf("Addresses at different offsets:\n");
  for (u64 offset : offsets) {
    u64 addr = base_addr + offset;
    printf("  Offset +%-10lu bytes:\n", offset);
    printAddressTableHeader();
    printAddress(addr, PageSize::Size4K);
  }

  freeBuffer(buffer, BUFFER_SIZE);
}

// Analyze large allocation spanning multiple 2MB pages
static void analyzeMultiPageAllocation() {
  printf("\n=== Multi-Page Allocation (10MB) ===\n");
  printf("Purpose: Analyze how large allocations span multiple 2MB pages\n");
  printf("Observe: Page table structure across page boundaries\n\n");

  const u64 BUFFER_SIZE = 10 * 1024 * 1024; // 10MB
  void *buffer = allocateBuffer(BUFFER_SIZE);

  if (!buffer) {
    fprintf(stderr, "ERROR: Could not allocate buffer\n");
    return;
  }

  u64 base_addr = (u64)buffer;
  printf("Allocated 10MB buffer at: 0x%016lx\n", base_addr);
  printf("Number of 2M pages: %lu\n", BUFFER_SIZE / (2 * 1024 * 1024));
  printf("Plus partial page at end\n\n");

  // Check boundaries
  u64 offsets[] = {0, 2 * 1024 * 1024, 4 * 1024 * 1024, 8 * 1024 * 1024,
                   10 * 1024 * 1024 - 1};

  printf("Page structure across buffer (showing 2M page transitions):\n");
  for (u64 offset : offsets) {
    u64 addr = base_addr + offset;
    printf("  Offset +%-10lu bytes:\n", offset);
    printAddressTableHeader();
    printAddress(addr, PageSize::Size4K);
  }

  freeBuffer(buffer, BUFFER_SIZE);
}

int main() {
  printf("Pointer Decomposition Analysis Tool\n");
  printf("====================================\n");

  analyzeSequentialAllocations();
  analyzeOSAllocations();
  analyzePointerDifferences();
  analyzeMultiPageAllocation();

  printf("\n====================================\n");
  printf("Analysis complete.\n");

  return 0;
}
