#pragma once

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
  int parentIndex;
};

struct GlobalProfiler {
  static constexpr size_t MAX_ENTRIES = 256;
  static constexpr size_t MAX_PARENT_STACK = 32;
  
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
  
  void begin(u64 freq = 0) {
    entryCount = 0;
    parentStackDepth = 0;
    cpuFreq = freq ? freq : EstimateCPUFreq(100);
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
          printf("%s: %llu cycles (%.2f%% excl, %.2f%% incl) [%llu hits]\n",
                 entries[i].label,
                 (unsigned long long)exclusiveCycles,
                 exclusivePct,
                 inclusivePct,
                 (unsigned long long)entries[i].hitCount);
        } else {
          printf("%s: %llu cycles (%.2f%%) [%llu hits]\n",
                 entries[i].label,
                 (unsigned long long)entries[i].elapsedCycles,
                 inclusivePct,
                 (unsigned long long)entries[i].hitCount);
        }
        
        // Recursively print children
        printEntriesByDepth(depth + 1, i);
      }
    }
  }
};

inline void BeginProfile(u64 cpuFreq = 0) {
  GlobalProfiler::instance().begin(cpuFreq);
}

inline void EndAndPrintProfile() {
  GlobalProfiler::instance().printResults();
}

// RAII-based scope timer with nesting support
struct ScopedTimer {
  int entryIndex;
  u64 startCycles;
  bool enabled;
  
  ScopedTimer(const char* label, bool en = true) : entryIndex(-1), startCycles(0), enabled(en) {
    if (enabled) {
      GlobalProfiler& prof = GlobalProfiler::instance();
      entryIndex = prof.addEntry(label);
      prof.pushParent(entryIndex);
      startCycles = ReadCPUTimer();
    }
  }
  
  ~ScopedTimer() {
    if (enabled && entryIndex >= 0) {
      u64 elapsed = ReadCPUTimer() - startCycles;
      GlobalProfiler::instance().recordElapsed(entryIndex, elapsed);
      GlobalProfiler::instance().popParent();
    }
  }
};

// Macro for easy scope timing - automatically detects if profiling is enabled
#define TIME_BLOCK(label) ScopedTimer _scoped_timer_##__LINE__(label, GlobalProfiler::instance().startTime != 0)
