
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

class Decoder {
public:
  // Decode all instructions (legacy function)
  static std::string assembleInstructions(const std::vector<char> &bytestream);

  // Decode a single instruction at the given offset
  // Returns a pair of (decoded instruction string, next byte offset)
  static std::pair<std::string, uint32_t>
  decodeOneInstruction(const std::vector<char> &bytestream, uint32_t offset);
};
