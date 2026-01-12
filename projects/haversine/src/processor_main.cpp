#include "haversine/processor.hpp"

#include <cstdlib>
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: haversine_processor <json_file> [binary_file]\n";
    return 1;
  }

  std::string jsonFile = argv[1];
  std::string binaryFile;

  if (argc >= 3) {
    binaryFile = argv[2];
  }

  try {
    HaversineProcessor processor;

    std::cout << "Parsing JSON file: " << jsonFile << "\n\n";
    processor.parseJSON(jsonFile);

    std::cout << "Computing haversine distances...\n\n";
    processor.computeDistances();

    if (!binaryFile.empty()) {
      double referenceAverage = processor.readBinaryReference(binaryFile);
      processor.compareWithReference(referenceAverage);
    } else {
      std::cout << "Sum:     " << processor.getSum() << "\n";
      std::cout << "Average: " << processor.getAverage() << "\n";
    }

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
