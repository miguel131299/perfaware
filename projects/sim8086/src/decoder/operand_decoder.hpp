#pragma once

#include <cstdint>
#include <string>
#include <vector>

//-----------------------------------------------------------------------------
// Operand types and structures
enum class OperandType { REGISTER, IMMEDIATE, MEMORY };

struct Operand {
  std::string value;
  OperandType type;
};

//-----------------------------------------------------------------------------
// Operand decoder interface
class OperandDecoder {
public:
  // Decode register from register/memory byte
  static Operand decodeRegister(uint8_t byte, bool getReg,
                                bool isWordOperation);

  // Decode immediate value (8-bit or 16-bit signed)
  static Operand decodeImmediate(const std::vector<uint8_t> &bytes,
                                 uint32_t offset, bool is16Bit);

  // Decode effective address with base registers
  static Operand decodeEffectiveAddress(uint8_t RM,
                                        const std::vector<uint8_t> &bytes,
                                        uint32_t base, bool isWithDisplacement);
};
