#pragma once

#include "haversine/platform_metrics.hpp"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

struct ProfileEntry {
  char label[64];
  u64 elapsedCycles;
  u64 hitCount;
};

struct GlobalProfiler {
  static constexpr size_t MAX_ENTRIES = 256;
  
  ProfileEntry entries[MAX_ENTRIES];
  size_t entryCount = 0;
  u64 startTime = 0;
  u64 totalTime = 0;
  u64 cpuFreq = 0;
  
  static GlobalProfiler& instance() {
    static GlobalProfiler g;
    return g;
  }
  
  void begin(u64 freq = 0) {
    entryCount = 0;
    cpuFreq = freq ? freq : EstimateCPUFreq(100);
    startTime = ReadCPUTimer();
  }
  
  void addEntry(const char* label, u64 elapsedCycles) {
    if (entryCount >= MAX_ENTRIES) return;
    
    // Check if entry already exists
    for (size_t i = 0; i < entryCount; ++i) {
      if (std::strcmp(entries[i].label, label) == 0) {
        entries[i].elapsedCycles += elapsedCycles;
        entries[i].hitCount++;
        return;
      }
    }
    
    // Add new entry
    std::strncpy(entries[entryCount].label, label, sizeof(entries[entryCount].label) - 1);
    entries[entryCount].label[sizeof(entries[entryCount].label) - 1] = '\0';
    entries[entryCount].elapsedCycles = elapsedCycles;
    entries[entryCount].hitCount = 1;
    entryCount++;
  }
  
  void printResults() {
    totalTime = ReadCPUTimer() - startTime;
    
    printf("\nProfiler Results:\n");
    printf("Total time: %.4fms (CPU freq %llu)\n", 
           (double)totalTime / (double)cpuFreq * 1000.0, 
           (unsigned long long)cpuFreq);
    
    for (size_t i = 0; i < entryCount; ++i) {
      double pct = totalTime > 0 ? (double)entries[i].elapsedCycles / (double)totalTime * 100.0 : 0.0;
      printf("  %s: %llu cycles (%.2f%%) [%llu hits]\n",
             entries[i].label,
             (unsigned long long)entries[i].elapsedCycles,
             pct,
             (unsigned long long)entries[i].hitCount);
    }
  }
};

inline void BeginProfile(u64 cpuFreq = 0) {
  GlobalProfiler::instance().begin(cpuFreq);
}

inline void EndAndPrintProfile() {
  GlobalProfiler::instance().printResults();
}

// RAII-based scope timer
struct ScopedTimer {
  const char* label;
  u64 startCycles;
  bool enabled;
  
  ScopedTimer(const char* lbl, bool en = true) : label(lbl), startCycles(0), enabled(en) {
    if (enabled) {
      startCycles = ReadCPUTimer();
    }
  }
  
  ~ScopedTimer() {
    if (enabled) {
      u64 elapsed = ReadCPUTimer() - startCycles;
      GlobalProfiler::instance().addEntry(label, elapsed);
    }
  }
};

// Macro for easy scope timing - pass false to disable profiling overhead
#define TIME_BLOCK(label) ScopedTimer _scoped_timer_##__LINE__(label, GlobalProfiler::instance().startTime != 0)
