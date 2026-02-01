#pragma once

#include "common/platform_metrics.hpp"
#include "common/types.hpp"

#include <climits>
#include <cstdio>

struct RepetitionTester {
  enum TestMode {
    Uninitialized,
    Testing,
    Completed,
    Error,
  };

  TestMode testMode = Uninitialized;

  u64 targetProcessedByteCount = 0;
  u64 cpuTimerFreq = 0;
  u64 tryForTime = 0; // In CPU cycles

  u64 testsStartedAt = 0;

  // Current iteration state
  u64 openBlockCount = 0;
  u64 closeBlockCount = 0;
  u64 timeAccumulated = 0;
  u64 pageFaultsAccumulated = 0;
  u64 bytesAccumulated = 0;

  // Results
  u64 testCount = 0;
  u64 totalTime = 0;
  u64 maxTime = 0;
  u64 minTime = UINT64_MAX;
  u64 totalPageFaults = 0;
  u64 minPageFaults = UINT64_MAX;
  u64 maxPageFaults = 0;

  bool isFirstIteration = true;

  void newTestWave(u64 targetByteCount, u64 tryForTimeMs = 10000) {
    testMode = Testing;
    targetProcessedByteCount = targetByteCount;
    cpuTimerFreq = EstimateCPUFreq(100);

    // Convert milliseconds to cycles
    tryForTime = (cpuTimerFreq / 1000) * tryForTimeMs;
    testsStartedAt = ReadCPUTimer();

    testCount = 0;
    totalTime = 0;
    maxTime = 0;
    minTime = UINT64_MAX;
    totalPageFaults = 0;
    minPageFaults = UINT64_MAX;
    maxPageFaults = 0;
    isFirstIteration = true;

    InitPageFaultTracking();

    openBlockCount = 0;
    closeBlockCount = 0;
    timeAccumulated = 0;
    pageFaultsAccumulated = 0;
    bytesAccumulated = 0;
  }

  void beginTime() {
    openBlockCount++;
    timeAccumulated -= ReadCPUTimer();
    pageFaultsAccumulated -= ReadPageFaultCount();
  }

  void endTime() {
    closeBlockCount++;
    timeAccumulated += ReadCPUTimer();
    pageFaultsAccumulated += ReadPageFaultCount();
  }

  void countBytes(u64 byteCount) { bytesAccumulated += byteCount; }

  bool isTesting() {
    if (testMode != Testing)
      return false;

    // Skip validation on first iteration (no work done yet)
    if (isFirstIteration) {
      isFirstIteration = false;
      return true;
    }

    // Check balanced blocks
    if (openBlockCount != closeBlockCount) {
      fprintf(stderr, "ERROR: Unbalanced open/close blocks\n");
      testMode = Error;
      return false;
    }

    // Check byte count
    if (bytesAccumulated != targetProcessedByteCount) {
      fprintf(stderr, "ERROR: Byte count mismatch (expected %llu, got %llu)\n",
              (unsigned long long)targetProcessedByteCount,
              (unsigned long long)bytesAccumulated);
      testMode = Error;
      return false;
    }

    // Update results
    testCount++;
    totalTime += timeAccumulated;
    totalPageFaults += pageFaultsAccumulated;

    if (timeAccumulated > maxTime) {
      maxTime = timeAccumulated;
    }

    if (pageFaultsAccumulated > maxPageFaults) {
      maxPageFaults = pageFaultsAccumulated;
    }

    if (timeAccumulated < minTime) {
      minTime = timeAccumulated;
      if (pageFaultsAccumulated < minPageFaults) {
        minPageFaults = pageFaultsAccumulated;
      }
      testsStartedAt = ReadCPUTimer(); // Reset trial timer on new minimum
      printResult();
    }

    // Check if we should stop (no new minimum for tryForTime)
    u64 elapsedSinceLastMin = ReadCPUTimer() - testsStartedAt;
    if (elapsedSinceLastMin > tryForTime) {
      testMode = Completed;
      printFinalResults();
      return false;
    }

    // Reset for next iteration
    openBlockCount = 0;
    closeBlockCount = 0;
    timeAccumulated = 0;
    pageFaultsAccumulated = 0;
    bytesAccumulated = 0;

    return true;
  }

private:
  void printResult() {
    double seconds = (double)minTime / (double)cpuTimerFreq;
    double mb = (double)targetProcessedByteCount / (1024.0 * 1024.0);
    double gbps = mb / seconds / 1024.0;

    printf("  Test %4llu: %.3fmb in %.6fs = %.2fgb/s (%llu page faults)\n",
           (unsigned long long)testCount, mb, seconds, gbps,
           (unsigned long long)minPageFaults);
  }

  void printFinalResults() {
    printf("\n=== Repetition Test Results ===\n");
    printf("Total tests run: %llu\n", (unsigned long long)testCount);

    double minSeconds = (double)minTime / (double)cpuTimerFreq;
    double maxSeconds = (double)maxTime / (double)cpuTimerFreq;
    double avgSeconds = (double)(totalTime / testCount) / (double)cpuTimerFreq;

    printf("Min time: %.6fs (%.3fms) [%llu cycles]\n", minSeconds,
           minSeconds * 1000.0, (unsigned long long)minTime);
    printf("Max time: %.6fs (%.3fms) [%llu cycles]\n", maxSeconds,
           maxSeconds * 1000.0, (unsigned long long)maxTime);
    u64 avgTime = testCount > 0 ? totalTime / testCount : 0;
    printf("Avg time: %.6fs (%.3fms) [%llu cycles]\n", avgSeconds,
           avgSeconds * 1000.0, (unsigned long long)avgTime);

    printf("\nPage Faults:\n");
    printf("  Min: %llu\n", (unsigned long long)minPageFaults);
    printf("  Max: %llu\n", (unsigned long long)maxPageFaults);
    u64 avgPageFaults = testCount > 0 ? totalPageFaults / testCount : 0;
    printf("  Avg: %llu\n", (unsigned long long)avgPageFaults);

    double mb = (double)targetProcessedByteCount / (1024.0 * 1024.0);
    double minGbps = mb / minSeconds / 1024.0;
    double maxGbps = mb / maxSeconds / 1024.0;
    double avgGbps = mb / avgSeconds / 1024.0;

    printf("\nThroughput:\n");
    printf("  Best: %.2f gb/s (%.3fmb)\n", minGbps, mb);
    printf("  Worst: %.2f gb/s\n", maxGbps);
    printf("  Average: %.2f gb/s\n", avgGbps);

    printf("\nBytes per Page Fault:\n");
    if (minPageFaults > 0) {
      double bytesPerMinPF =
          (double)targetProcessedByteCount / (double)minPageFaults;
      double bytesPerMaxPF =
          (double)targetProcessedByteCount / (double)maxPageFaults;
      double bytesPerAvgPF = (double)targetProcessedByteCount /
                             (double)(avgPageFaults > 0 ? avgPageFaults : 1);
      printf("  Best: %.0f bytes/fault\n", bytesPerMinPF);
      printf("  Worst: %.0f bytes/fault\n", bytesPerMaxPF);
      printf("  Average: %.0f bytes/fault\n", bytesPerAvgPF);
    } else {
      printf("  (No page faults recorded)\n");
    }
    printf("===============================\n\n");
  }
};

// Convenience macros for repetition testing
#define REPETITION_TEST_BEGIN(tester) while (tester.isTesting())

#define REPETITION_TEST_START_TIMING(tester) tester.beginTime()

#define REPETITION_TEST_END_TIMING(tester) tester.endTime()

#define REPETITION_TEST_COUNT_BYTES(tester, byteCount)                         \
  tester.countBytes(byteCount)
