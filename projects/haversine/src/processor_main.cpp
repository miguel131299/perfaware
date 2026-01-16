#include "haversine/processor.hpp"
#include "haversine/platform_metrics.hpp"
#include "haversine/profiler.hpp"

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <fstream>

int main(int argc, char *argv[]) {
  // Quick check for profiling flag
  bool enableProfiling = false;
  for (int i = 2; i < argc; ++i) {
    if (std::string(argv[i]) == "--profile" || std::string(argv[i]) == "-p") {
      enableProfiling = true;
      break;
    }
  }
  
  if (enableProfiling) {
    BeginProfile();
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

  try {
    HaversineProcessor processor;

    std::string jsonContent;
    {
      TIME_BLOCK("Read");
      jsonContent = processor.readJSONFile(jsonFile);
    }
    
    u64 inputSize = jsonContent.size();
    
    {
      TIME_BLOCK("Parse");
      processor.parseJSONString(jsonContent);
    }

    {
      TIME_BLOCK("Sum");
      processor.computeDistances();
    }

    {
      TIME_BLOCK("Output");
      if (!binaryFile.empty()) {
        double referenceAverage = processor.readBinaryReference(binaryFile);
        processor.compareWithReference(referenceAverage);
      } else {
        printf("Haversine sum: %.16f\n", processor.getSum());
      }
    }

    if (enableProfiling) {
      u64 pairCount = processor.getDistances().size();
      printf("\nInput size: %llu\n", (unsigned long long)inputSize);
      printf("Pair count: %llu\n", (unsigned long long)pairCount);
      EndAndPrintProfile();
    }

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
