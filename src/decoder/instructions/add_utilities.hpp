#pragma once

#include <cstdint>
#include <string>

//-----------------------------------------------------------------------------
// ADD-specific utility functions and formatters

// Output ADD instruction to stringstream
inline void outputADDInstruction(std::stringstream &ss, const std::string &dest,
                                 const std::string &src) {
  ss << "add " << dest << ", " << src << "\n";
}
