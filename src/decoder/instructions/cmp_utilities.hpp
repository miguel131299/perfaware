#pragma once

#include <cstdint>
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
// CMP-specific utility functions and formatters

// Output CMP instruction to stringstream
inline void outputCMPInstruction(std::stringstream &ss, const std::string &dest,
                                 const std::string &src) {
  ss << "cmp " << dest << ", " << src << "\n";
}
