#include "common/platform_metrics.hpp"
#include "common/profiler.hpp"
#include "haversine/processor.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: haversine_processor <json_file> [binary_file]\n";
    return 1;
  }

  std::string jsonFile = argv[1];
  std::string binaryFile;

  // Parse arguments for binary file
  for (int i = 2; i < argc; ++i) {
    if (binaryFile.empty()) {
      binaryFile = argv[i];
    }
  }

  BeginProfile();

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

    u64 pairCount = processor.getDistances().size();
    printf("\nInput size: %llu\n", (unsigned long long)inputSize);
    printf("Pair count: %llu\n", (unsigned long long)pairCount);
    EndAndPrintProfile();

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
