// main.cpp
#include <exception>
#include <fstream>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <vector>

std::vector<char> readBinaryFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);

  if (!file) {
    throw std::runtime_error("Could not open file: " + filename);
  }

  // Determine the file size
  file.seekg(0, std::ios::end);
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  // Read file into buffer
  std::vector<char> buffer(size);
  if (!file.read(buffer.data(), size)) {
    throw std::runtime_error("Error reading file: " + filename);
  }

  return buffer;
}

int main(int argc, char *argv[]) {

  // Program also counts as argument, so we actually need 2 arguments
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <binary_file>\n";
    return 1;
  }

  const std::string filename = argv[1];

  try {
    std::vector<char> data = readBinaryFile(filename);
    std::cout << "Read " << data.size() << " bytes from " << filename << "\n";
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}
