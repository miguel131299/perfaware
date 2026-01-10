#pragma once

#include "instructions/utilities/bit_utilities.hpp"
#include <cstdint>
#include <format>
#include <sstream>
#include <stdexcept>
#include <string>

//-----------------------------------------------------------------------------
// MOD field encoding
enum class MODEncoding : uint8_t {
  MEMORY_MODE_NO_DISPLACEMENT,
  MEMORY_MODE_8_DISPLACEMENT,
  MEMORY_MODE_16_DISPLACEMENT,
  REGISTER_MODE,
};

//-----------------------------------------------------------------------------
inline MODEncoding getMODEncoding(uint8_t byte) {
  byte = (byte & BIT_FIELD_MOD_MASK) >> 6;
  switch (byte) {
  case 0:
    return MODEncoding::MEMORY_MODE_NO_DISPLACEMENT;
  case 1:
    return MODEncoding::MEMORY_MODE_8_DISPLACEMENT;
  case 2:
    return MODEncoding::MEMORY_MODE_16_DISPLACEMENT;
  case 3:
    return MODEncoding::REGISTER_MODE;
  default:
    throw std::runtime_error("Error decoding MOD: " + std::to_string(byte));
  }
}

//-----------------------------------------------------------------------------
// Helper to format and output MOV instruction
inline void outputMOVInstruction(std::stringstream &ss, const std::string &dest,
                                 const std::string &src) {
  ss << std::format("MOV {}, {}\n", dest, src);
}

//-----------------------------------------------------------------------------
// Helper to build base register address string from RM field
inline std::string getBaseRegisterAddress(uint8_t rmBits) {
  switch (rmBits) {
  case 0:
    return "BX + SI";
  case 1:
    return "BX + DI";
  case 2:
    return "BP + SI";
  case 3:
    return "BP + DI";
  case 4:
    return "SI";
  case 5:
    return "DI";
  case 6:
    return "BP";
  case 7:
    return "BX";
  default:
    return "";
  }
}

//-----------------------------------------------------------------------------
// Helper to build displacement string with proper +/- formatting
inline std::string formatDisplacement(const std::string &baseAddress,
                                      int16_t displacement) {
  if (displacement == 0) {
    return "[" + baseAddress + "]";
  }

  if (displacement < 0) {
    return "[" + baseAddress + " - " + std::to_string(-displacement) + "]";
  }

  return "[" + baseAddress + " + " + std::to_string(displacement) + "]";
}