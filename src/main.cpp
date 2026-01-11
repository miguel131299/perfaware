// main.cpp
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <sim8086/decoder.hpp>
#include <sim8086/simulator.hpp>
#include <sim8086/utils.hpp>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {

  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <binary_file> [--exec] [--dump] [--dump-binary]\n";
    std::cerr << "  --exec         Execute the program and show simulator state\n";
    std::cerr << "  --dump         Dump memory content after execution (hex format)\n";
    std::cerr << "  --dump-binary  Dump raw 64KB memory to stdout (for piping)\n";
    return 1;
  }

  std::string filename = argv[1];
  bool executeProgram = false;
  bool dumpMemory = false;
  bool dumpBinary = false;

  // Parse flags
  for (int i = 2; i < argc; i++) {
    std::string flag = argv[i];
    if (flag == "--exec") {
      executeProgram = true;
    } else if (flag == "--dump") {
      dumpMemory = true;
      executeProgram = true; // --dump implies execution
    } else if (flag == "--dump-binary") {
      dumpBinary = true;
      executeProgram = true; // --dump-binary implies execution
    } else {
      std::cerr << "Unknown flag: " << flag << "\n";
      return 1;
    }
  }

  // If filename is not an absolute path, prepend LISTINGS_DIR environment variable
  if (filename[0] != '/') {
    const char *listingsDir = std::getenv("LISTINGS_DIR");
    if (listingsDir) {
      filename = std::string(listingsDir) + "/" + filename;
    }
  }

  try {
    std::vector<char> data = Utils::readBinaryFile(filename);

    if (executeProgram) {
      // Execute the program
      Simulator simulator(data);
      simulator.run();
      
      if (dumpBinary) {
        // Output raw memory to stdout (for piping to file)
        std::string binaryData = simulator.dumpMemoryRaw();
        std::cout.write(binaryData.c_str(), binaryData.size());
      } else {
        // Show execution trace
        std::cout << simulator.getTrace();
        
        // Show simulator state
        std::cout << "\n" << simulator.dumpState();
        
        // Optionally dump memory content (hex format)
        if (dumpMemory) {
          std::cout << "\n" << simulator.dumpMemory();
        }
      }
    } else {
      // Default: Decode and show assembly
      std::string instructions = Decoder::assembleInstructions(data);
      std::cout << instructions << std::endl;
    }
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}
