#include "haversine/processor.hpp"
#include "haversine/platform_metrics.hpp"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>

int main(int argc, char *argv[]) {
  // Capture startup timer at the very beginning if profiling
  u64 StartupStart = 0;
  bool enableProfiling = false;
  
  // Quick check for profiling flag to start timer early
  for (int i = 2; i < argc; ++i) {
    if (std::string(argv[i]) == "--profile" || std::string(argv[i]) == "-p") {
      enableProfiling = true;
      break;
    }
  }
  
  if (enableProfiling) {
    StartupStart = ReadCPUTimer();
  }

  if (argc < 2) {
    std::cerr << "Usage: haversine_processor <json_file> [binary_file] [--profile]\n";
    return 1;
  }

  std::string jsonFile = argv[1];
  std::string binaryFile;

  // Parse arguments for binary file and --profile flag
  for (int i = 2; i < argc; ++i) {
    if (std::string(argv[i]) == "--profile" || std::string(argv[i]) == "-p") {
      enableProfiling = true;
    } else if (binaryFile.empty()) {
      binaryFile = argv[i];
    }
  }

  // Estimate CPU frequency if profiling enabled
  u64 CPUFreq = 0;
  if (enableProfiling) {
    CPUFreq = EstimateCPUFreq(100);
  }

  try {
    HaversineProcessor processor;

    // Timing markers
    u64 StartupEnd = 0, ReadStart = 0, ReadEnd = 0, ParseStart = 0, ParseEnd = 0, SumStart = 0, SumEnd = 0, MiscOutputStart = 0, TotalEnd = 0;
    if (enableProfiling) {
      StartupEnd = ReadCPUTimer();
      ReadStart = ReadCPUTimer();
    }

    std::string jsonContent = processor.readJSONFile(jsonFile);
    
    // Get input file size from the content we just read
    u64 inputSize = jsonContent.size();

    if (enableProfiling) {
      ReadEnd = ReadCPUTimer();
      ParseStart = ReadCPUTimer();
    }

    processor.parseJSONString(jsonContent);

    if (enableProfiling) {
      ParseEnd = ReadCPUTimer();
      SumStart = ReadCPUTimer();
    }

    processor.computeDistances();

    if (enableProfiling) {
      SumEnd = ReadCPUTimer();
      MiscOutputStart = ReadCPUTimer();
    }

    if (!binaryFile.empty()) {
      double referenceAverage = processor.readBinaryReference(binaryFile);
      processor.compareWithReference(referenceAverage);
    } else {
      printf("Haversine sum: %.16f\n", processor.getSum());
    }

    if (enableProfiling) {
      TotalEnd = ReadCPUTimer();

      // Calculate elapsed cycles
      u64 StartupElapsed = StartupEnd - StartupStart;
      u64 ReadElapsed = ReadEnd - ReadStart;
      u64 ParseElapsed = ParseEnd - ParseStart;
      u64 SumElapsed = SumEnd - SumStart;
      u64 MiscOutputElapsed = TotalEnd - MiscOutputStart;
      u64 TotalElapsed = TotalEnd - StartupStart;

      // Get pair count from processor
      u64 pairCount = processor.getDistances().size();

      // Convert to milliseconds for display
      double totalMS = (double)TotalElapsed / (double)CPUFreq * 1000.0;

      // Calculate percentages
      double startupPct = TotalElapsed > 0 ? (double)StartupElapsed / (double)TotalElapsed * 100.0 : 0.0;
      double readPct = TotalElapsed > 0 ? (double)ReadElapsed / (double)TotalElapsed * 100.0 : 0.0;
      double parsePct = TotalElapsed > 0 ? (double)ParseElapsed / (double)TotalElapsed * 100.0 : 0.0;
      double sumPct = TotalElapsed > 0 ? (double)SumElapsed / (double)TotalElapsed * 100.0 : 0.0;
      double miscOutputPct = TotalElapsed > 0 ? (double)MiscOutputElapsed / (double)TotalElapsed * 100.0 : 0.0;

      // Print profiling results
      printf("\nInput size: %llu\n", (unsigned long long)inputSize);
      printf("Pair count: %llu\n\n", (unsigned long long)pairCount);

      printf("Total time: %.4fms (CPU freq %llu)\n", totalMS, (unsigned long long)CPUFreq);
      printf("  Startup: %llu (%.2f%%)\n", (unsigned long long)StartupElapsed, startupPct);
      printf("  Read: %llu (%.2f%%)\n", (unsigned long long)ReadElapsed, readPct);
      printf("  Parse: %llu (%.2f%%)\n", (unsigned long long)ParseElapsed, parsePct);
      printf("  Sum: %llu (%.2f%%)\n", (unsigned long long)SumElapsed, sumPct);
      printf("  MiscOutput: %llu (%.2f%%)\n", (unsigned long long)MiscOutputElapsed, miscOutputPct);
    }

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
