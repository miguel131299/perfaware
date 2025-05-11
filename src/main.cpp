// main.cpp
#include <exception>
#include <iostream>
#include <sim8086/decoder.hpp>
#include <sim8086/utils.hpp>
#include <string>
#include <vector>

int main(int argc, char *argv[]) {

  // Program also counts as argument, so we actually need 2 arguments
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <binary_file>\n";
    return 1;
  }

  const std::string filename = argv[1];

  try {
    std::vector<char> data = Utils::readBinaryFile(filename);
    std::string instructions = Decoder::assembleInstructions(data);
    std::cout << instructions << std::endl;
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}
