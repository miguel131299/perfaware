#include "operand_decoder.hpp"

#include "instructions/utilities/bit_utilities.hpp"
#include "tables/registers.hpp"
#include <stdexcept>
#include <string>

//-----------------------------------------------------------------------------
Operand OperandDecoder::decodeRegister(uint8_t byte, bool getReg,
                                       bool isWordOperation) {
  uint8_t regCode;
  if (getReg) {
    regCode = (byte & BIT_FIELD_REG_MASK) >> 3; // REG
  } else {
    regCode = byte & BIT_FIELD_RM_MASK; // R/M
  }

  return {std::string(getRegisterName(regCode, isWordOperation)),
          OperandType::REGISTER};
}

//-----------------------------------------------------------------------------
Operand OperandDecoder::decodeImmediate(const std::vector<char> &bytestream,
                                        uint32_t offset, bool is16Bit) {
  std::string value;

  if (is16Bit) {
    int16_t immediateValue = readInt16(bytestream, offset);
    value = std::to_string(immediateValue);
  } else {
    int8_t immediateValue = static_cast<int8_t>(bytestream[offset]);
    value = std::to_string(immediateValue);
  }

  return {value, OperandType::IMMEDIATE};
}

//-----------------------------------------------------------------------------
static const char *getEffectiveAddressBase(uint8_t RM) {
  switch (RM) {
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
    throw std::runtime_error("Invalid RM code: " + std::to_string(RM));
  }
}

Operand
OperandDecoder::decodeEffectiveAddress(uint8_t RM,
                                       const std::vector<char> &bytestream,
                                       uint32_t base, bool isWithDisplacement) {
  std::string result = getEffectiveAddressBase(RM);

  // Add displacement if present
  if (isWithDisplacement) {
    int16_t displacement = readInt16(bytestream, base + 2);

    if (displacement != 0) {
      result += " + " + std::to_string(displacement);
    }
  }

  return {"[" + result + "]", OperandType::MEMORY};
}
