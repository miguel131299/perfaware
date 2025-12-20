#pragma once

#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

//-----------------------------------------------------------------------------
// Register lookup tables
static constexpr std::array<std::string_view, 8> WORD_REGISTERS = {
    "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};

static constexpr std::array<std::string_view, 8> BYTE_REGISTERS = {
    "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};

inline std::string_view getRegisterName(uint8_t regCode, bool isWordOperation) {
  if (regCode > 7) {
    throw std::runtime_error("Invalid register code: " +
                             std::to_string(regCode));
  }
  return isWordOperation ? WORD_REGISTERS[regCode] : BYTE_REGISTERS[regCode];
}
