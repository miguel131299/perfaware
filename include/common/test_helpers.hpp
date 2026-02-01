#pragma once

#include "common/types.hpp"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <sys/mman.h>

enum AllocationType {
  AllocType_None,
  AllocType_Malloc,
  AllocType_HugePages,
  AllocType_Count,
};

struct BaseTestParameters {
  u64 bufferSize;
  char *buffer;
  AllocationType allocType;
  u64 testTimeMs;
  void *mmapHandle;
  u64 mmapSize;
};

struct TestParameters : BaseTestParameters {};

struct ReadParameters : BaseTestParameters {
  const char *filename;
  u64 fileSize;
};

static void handleAllocation(BaseTestParameters *params, char **buffer) {
  switch (params->allocType) {
  case AllocType_None:
    break;

  case AllocType_Malloc:
    *buffer = (char *)malloc(params->bufferSize);
    if (!*buffer) {
      fprintf(stderr, "ERROR: Could not allocate buffer\n");
    }
    break;

  case AllocType_HugePages: {
#ifdef MAP_HUGETLB
    const u64 HUGE_PAGE_SIZE = 2 * 1024 * 1024;
    u64 allocSize =
        ((params->bufferSize + HUGE_PAGE_SIZE - 1) / HUGE_PAGE_SIZE) *
        HUGE_PAGE_SIZE;

    *buffer = (char *)mmap(nullptr, allocSize, PROT_READ | PROT_WRITE,
                           MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);
    if (*buffer == MAP_FAILED) {
      fprintf(stderr, "ERROR: Could not allocate huge pages (size: %llu MB)\n",
              (unsigned long long)(allocSize / (1024 * 1024)));
      *buffer = nullptr;
      params->mmapHandle = nullptr;
      params->mmapSize = 0;
    } else {
      params->mmapHandle = *buffer;
      params->mmapSize = allocSize;
    }
#else
    fprintf(stderr, "ERROR: Huge pages not supported on this platform\n");
    *buffer = nullptr;
    params->mmapHandle = nullptr;
    params->mmapSize = 0;
#endif
  } break;

  default:
    fprintf(stderr, "ERROR: Unrecognized allocation type\n");
    break;
  }
}

static void handleDeallocation(BaseTestParameters *params, char **buffer) {
  switch (params->allocType) {
  case AllocType_None:
    break;

  case AllocType_Malloc:
    if (*buffer) {
      free(*buffer);
      *buffer = nullptr;
    }
    break;

  case AllocType_HugePages:
    if (params->mmapHandle != nullptr && params->mmapSize > 0) {
      if (munmap(params->mmapHandle, params->mmapSize) != 0) {
        fprintf(stderr, "WARNING: Failed to munmap huge pages (errno: %d)\n",
                errno);
      }
      *buffer = nullptr;
      params->mmapHandle = nullptr;
      params->mmapSize = 0;
    }
    break;

  default:
    fprintf(stderr, "ERROR: Unrecognized allocation type\n");
    break;
  }
}
