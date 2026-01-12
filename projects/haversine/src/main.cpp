#include "haversine/haversine.hpp"

#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>

int main(int argc, char *argv[]) {
  uint64_t pairCount = 10;
  uint64_t seed = 0;
  bool useClustering = true;
  int gridSize = 2;
  bool writeBinary = false;
  bool writeJSON = false;
  std::string binaryOutput;
  std::string jsonOutput;

  // Parse command line arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-c" || arg == "--count") {
      if (i + 1 < argc) {
        pairCount = std::stoull(argv[++i]);
      }
    } else if (arg == "-s" || arg == "--seed") {
      if (i + 1 < argc) {
        seed = std::stoull(argv[++i]);
      }
    } else if (arg == "-g" || arg == "--grid-size") {
      if (i + 1 < argc) {
        gridSize = std::stoi(argv[++i]);
      }
    } else if (arg == "--no-clustering") {
      useClustering = false;
    } else if (arg == "-b" || arg == "--binary") {
      if (i + 1 < argc) {
        writeBinary = true;
        binaryOutput = argv[++i];
      }
    } else if (arg == "-j" || arg == "--json") {
      if (i + 1 < argc) {
        writeJSON = true;
        jsonOutput = argv[++i];
      }
    } else if (arg == "-h" || arg == "--help") {
      std::cout
          << "Haversine input data generator\n\n"
          << "Usage: haversine_gen [options]\n\n"
          << "Options:\n"
          << "  -c, --count N       Generate N point pairs (default: 10)\n"
          << "  -s, --seed S        Random seed (default: random)\n"
          << "  -g, --grid-size N   Clustering grid size NxN (default: 2, "
             "gives 4 clusters)\n"
          << "  --no-clustering     Disable clustering for random "
             "distribution\n"
          << "  -j, --json FILE     Write JSON output to FILE\n"
          << "  -b, --binary FILE   Write binary results to FILE\n"
          << "  -h, --help          Show this help message\n";
      return 0;
    }
  }

  try {
    HaversineGenerator gen(pairCount, seed, useClustering, gridSize);

    // Print metadata
    std::cout << "Method: " << (useClustering ? "cluster" : "random") << "\n";
    std::cout << "Random seed: " << gen.getSeed() << "\n";
    std::cout << "Pair count: " << gen.getPairs().size() << "\n";
    if (useClustering) {
      std::cout << "Grid size: " << gen.getGridSize() << "x"
                << gen.getGridSize() << " ("
                << (gen.getGridSize() * gen.getGridSize()) << " clusters)\n";
    }
    std::cout << std::fixed << std::setprecision(16);
    std::cout << "Expected sum: " << gen.getExpectedSum() << "\n\n";

    // Output JSON to file or stdout
    std::string jsonOutput_str = gen.toJSON();
    if (writeJSON) {
      std::ofstream jsonFile(jsonOutput);
      if (!jsonFile) {
        std::cerr << "Error: Failed to open JSON output file: " << jsonOutput
                  << "\n";
        return 1;
      }
      jsonFile << jsonOutput_str;
      jsonFile.close();
      std::cerr << "JSON output written to: " << jsonOutput << "\n";
    } else {
      std::cout << jsonOutput_str;
    }

    // Optionally write binary file
    if (writeBinary) {
      gen.writeBinaryResults(binaryOutput);
      std::cerr << "Binary results written to: " << binaryOutput << "\n";
    }

    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }
}
