
#pragma once

#include <string>
#include <vector>

class Decoder {
public:
  static std::string assembleInstructions(const std::vector<char> &bytestream);
};
