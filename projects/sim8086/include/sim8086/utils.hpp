#pragma once

#include <string>
#include <vector>

class Utils {
public:
  static std::vector<char> readBinaryFile(const std::string &filename);
};