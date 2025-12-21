#pragma once

#include <cstdint>
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
// SUB-specific utility functions and formatters

// Output SUB instruction to stringstream
inline void outputSUBInstruction(std::stringstream &ss, const std::string &dest,
                                 const std::string &src) {
  ss << "sub " << dest << ", " << src << "\n";
}
