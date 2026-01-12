#pragma once

#include <sstream>
#include <string>
#include <string_view>

//-----------------------------------------------------------------------------
// Generic instruction output formatting function

inline void outputInstruction(std::stringstream &ss, std::string_view mnemonic,
                              const std::string &dest, const std::string &src) {
  ss << mnemonic << " " << dest << ", " << src << "\n";
}
