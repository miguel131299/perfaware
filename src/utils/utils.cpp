#include <fstream>
#include <ios>
#include <sim8086/utils.hpp>

std::vector<char> Utils::readBinaryFile(const std::string &filename) {
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
