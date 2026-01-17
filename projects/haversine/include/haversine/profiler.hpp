#pragma once

// Set to 1 to enable profiling, 0 to disable (zero overhead when disabled)
#ifndef ENABLE_PROFILING
#define ENABLE_PROFILING 1
#endif

#if ENABLE_PROFILING

#include "haversine/platform_metrics.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

struct ProfileEntry {
  char label[64];
  u64 elapsedCycles;
  u64 elapsedChildrenCycles;
  u64 hitCount;
  u64 processedByteCount;
  int parentIndex;
};

struct GlobalProfiler {
  static constexpr size_t MAX_ENTRIES = 256;
  static constexpr size_t MAX_PARENT_STACK = 32;
  static constexpr double MEGABYTE = 1024.0 * 1024.0;
  static constexpr double GIGABYTE = MEGABYTE * 1024.0;
  
  ProfileEntry entries[MAX_ENTRIES];
  size_t entryCount = 0;
  int parentStack[MAX_PARENT_STACK];
  size_t parentStackDepth = 0;
  u64 startTime = 0;
  u64 totalTime = 0;
  u64 cpuFreq = 0;
  
  static GlobalProfiler& instance() {
    static GlobalProfiler g;
    return g;
  }
  
  void begin() {
    entryCount = 0;
    parentStackDepth = 0;
    cpuFreq = EstimateCPUFreq(100);
    startTime = ReadCPUTimer();
  }
  
  int addEntry(const char* label) {
    if (entryCount >= MAX_ENTRIES) return -1;
    
    // Check if entry already exists at this nesting level
    int parentIdx = parentStackDepth > 0 ? parentStack[parentStackDepth - 1] : -1;
    for (size_t i = 0; i < entryCount; ++i) {
      if (std::strcmp(entries[i].label, label) == 0 && entries[i].parentIndex == parentIdx) {
        return i;
      }
    }
    
    // Add new entry
    int newIndex = entryCount;
    std::strncpy(entries[newIndex].label, label, sizeof(entries[newIndex].label) - 1);
    entries[newIndex].label[sizeof(entries[newIndex].label) - 1] = '\0';
    entries[newIndex].elapsedCycles = 0;
    entries[newIndex].elapsedChildrenCycles = 0;
    entries[newIndex].hitCount = 0;
    entries[newIndex].processedByteCount = 0;
    entries[newIndex].parentIndex = parentIdx;
    entryCount++;
    
    return newIndex;
  }
  
  void recordElapsed(int entryIndex, u64 elapsedCycles) {
    if (entryIndex < 0 || entryIndex >= (int)entryCount) return;
    
    entries[entryIndex].elapsedCycles += elapsedCycles;
    entries[entryIndex].hitCount++;
    
    // Add to parent's children time
    int parentIdx = entries[entryIndex].parentIndex;
    if (parentIdx >= 0 && parentIdx < (int)entryCount) {
      entries[parentIdx].elapsedChildrenCycles += elapsedCycles;
    }
  }
  
  void recordBandwidth(int entryIndex, u64 byteCount) {
    if (entryIndex < 0 || entryIndex >= (int)entryCount) return;
    entries[entryIndex].processedByteCount += byteCount;
  }
  
  void pushParent(int entryIndex) {
    if (parentStackDepth < MAX_PARENT_STACK) {
      parentStack[parentStackDepth++] = entryIndex;
    }
  }
  
  void popParent() {
    if (parentStackDepth > 0) {
      parentStackDepth--;
    }
  }
  
  void printResults() {
    totalTime = ReadCPUTimer() - startTime;
    
    printf("\nProfiler Results:\n");
    printf("Total time: %.4fms (CPU freq %llu)\n", 
           (double)totalTime / (double)cpuFreq * 1000.0, 
           (unsigned long long)cpuFreq);
    
    // Print by nesting level
    printEntriesByDepth(0, -1);
    
    // Print aggregated results
    printAggregatedResults();
  }
  
  void printThroughput(u64 processedByteCount) {
    if (processedByteCount > 0) {
      double seconds = (double)processedByteCount / (double)cpuFreq;
      double bytesPerSecond = (double)processedByteCount / seconds;
      double megabytes = (double)processedByteCount / MEGABYTE;
      double gigabytesPerSecond = bytesPerSecond / GIGABYTE;
      printf(" - %.3fmb at %.2fgb/s", megabytes, gigabytesPerSecond);
    }
  }
  
private:
  void printEntriesByDepth(int depth, int parentIndex) {
    for (size_t i = 0; i < entryCount; ++i) {
      if (entries[i].parentIndex == parentIndex) {
        u64 exclusiveCycles = entries[i].elapsedCycles - entries[i].elapsedChildrenCycles;
        double exclusivePct = totalTime > 0 ? (double)exclusiveCycles / (double)totalTime * 100.0 : 0.0;
        double inclusivePct = totalTime > 0 ? (double)entries[i].elapsedCycles / (double)totalTime * 100.0 : 0.0;
        
        // Print indent
        for (int d = 0; d < depth; ++d) printf("  ");
        
        if (entries[i].elapsedChildrenCycles > 0) {
          printf("%s: %llu cycles (%.2f%% excl, %.2f%% incl) [%llu hits]",
                 entries[i].label,
                 (unsigned long long)exclusiveCycles,
                 exclusivePct,
                 inclusivePct,
                 (unsigned long long)entries[i].hitCount);
        } else {
          printf("%s: %llu cycles (%.2f%%) [%llu hits]",
                 entries[i].label,
                 (unsigned long long)entries[i].elapsedCycles,
                 inclusivePct,
                 (unsigned long long)entries[i].hitCount);
        }
        
        printThroughput(entries[i].processedByteCount);
        printf("\n");
        
        // Recursively print children
        printEntriesByDepth(depth + 1, i);
      }
    }
  }
  
  void printAggregatedResults() {
    printf("\n\nAggregated by block (exclusive times, all nesting levels):\n");
    
    // Aggregate exclusive times by label
    struct AggregatedEntry {
      char label[64];
      u64 totalExclusiveCycles = 0;
      u64 totalHits = 0;
      u64 totalProcessedBytes = 0;
    };
    
    AggregatedEntry aggregated[MAX_ENTRIES];
    size_t aggregatedCount = 0;
    
    for (size_t i = 0; i < entryCount; ++i) {
      u64 exclusiveCycles = entries[i].elapsedCycles - entries[i].elapsedChildrenCycles;
      
      // Find or create aggregated entry
      bool found = false;
      for (size_t j = 0; j < aggregatedCount; ++j) {
        if (std::strcmp(aggregated[j].label, entries[i].label) == 0) {
          aggregated[j].totalExclusiveCycles += exclusiveCycles;
          aggregated[j].totalHits += entries[i].hitCount;
          aggregated[j].totalProcessedBytes += entries[i].processedByteCount;
          found = true;
          break;
        }
      }
      
      if (!found && aggregatedCount < MAX_ENTRIES) {
        std::strncpy(aggregated[aggregatedCount].label, entries[i].label, sizeof(aggregated[aggregatedCount].label) - 1);
        aggregated[aggregatedCount].label[sizeof(aggregated[aggregatedCount].label) - 1] = '\0';
        aggregated[aggregatedCount].totalExclusiveCycles = exclusiveCycles;
        aggregated[aggregatedCount].totalHits = entries[i].hitCount;
        aggregated[aggregatedCount].totalProcessedBytes = entries[i].processedByteCount;
        aggregatedCount++;
      }
    }
    
    // Print aggregated results sorted by time (descending)
    for (size_t i = 0; i < aggregatedCount; ++i) {
      for (size_t j = i + 1; j < aggregatedCount; ++j) {
        if (aggregated[j].totalExclusiveCycles > aggregated[i].totalExclusiveCycles) {
          std::swap(aggregated[i], aggregated[j]);
        }
      }
    }
    
    for (size_t i = 0; i < aggregatedCount; ++i) {
      double pct = totalTime > 0 ? (double)aggregated[i].totalExclusiveCycles / (double)totalTime * 100.0 : 0.0;
      printf("  %s: %llu cycles (%.2f%%) [%llu hits]",
             aggregated[i].label,
             (unsigned long long)aggregated[i].totalExclusiveCycles,
             pct,
             (unsigned long long)aggregated[i].totalHits);
      
      printThroughput(aggregated[i].totalProcessedBytes);
      printf("\n");
    }
  }
};

inline void BeginProfile() {
  GlobalProfiler::instance().begin();
}

inline void EndAndPrintProfile() {
  GlobalProfiler::instance().printResults();
}

// RAII-based scope timer with optional bandwidth tracking
struct ScopedTimer {
  int entryIndex;
  u64 startCycles;
  u64 byteCount;
  
  ScopedTimer(const char* label, u64 bytes = 0) : entryIndex(-1), startCycles(0), byteCount(bytes) {
    GlobalProfiler& prof = GlobalProfiler::instance();
    entryIndex = prof.addEntry(label);
    prof.pushParent(entryIndex);
    if (byteCount > 0) {
      prof.recordBandwidth(entryIndex, byteCount);
    }
    startCycles = ReadCPUTimer();
  }
  
  ~ScopedTimer() {
    if (entryIndex >= 0) {
      u64 elapsed = ReadCPUTimer() - startCycles;
      GlobalProfiler::instance().recordElapsed(entryIndex, elapsed);
      GlobalProfiler::instance().popParent();
    }
  }
};

// Macro for easy scope timing
#define TIME_BLOCK(label) ScopedTimer _scoped_timer_##__LINE__(label)

// Macro for scope timing with bandwidth
#define TIME_BANDWIDTH(label, byteCount) ScopedTimer _scoped_timer_##__LINE__(label, byteCount)

#else

// When profiling is disabled, all macros become no-ops
inline void BeginProfile() {}
inline void EndAndPrintProfile() {}
#define TIME_BLOCK(label) (void)0
#define TIME_BANDWIDTH(label, byteCount) (void)0

#endif
