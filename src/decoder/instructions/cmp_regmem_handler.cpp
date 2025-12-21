#include "cmp_regmem_handler.hpp"

#include "../bit_utilities.hpp"
#include "../operand_decoder.hpp"
#include "../tables/registers.hpp"
#include "cmp_utilities.hpp"
#include <format>
#include <sstream>
#include <stdexcept>
#include <string>

//-----------------------------------------------------------------------------
uint32_t CMPRegMemHandler::handleAddressingMode(
    std::stringstream &ss, MODEncoding mod, uint8_t rmBits,
    const std::vector<char> &bytestream, uint32_t baseOffset, uint8_t regCode,
    bool isWordOperation) {
  std::string regOperand =
      std::string(getRegisterName(regCode, isWordOperation));

  switch (mod) {
  case MODEncoding::REGISTER_MODE: {
    std::string rmOperand =
        std::string(getRegisterName(rmBits, isWordOperation));
    outputCMPInstruction(ss, regOperand, rmOperand);
    return 2;
  }

  case MODEncoding::MEMORY_MODE_NO_DISPLACEMENT: {
    Operand memOperand = OperandDecoder::decodeEffectiveAddress(
        rmBits, bytestream, baseOffset, false);

    if (rmBits == 6) {
      int16_t address = readInt16(bytestream, baseOffset + 2);
      memOperand.value = "[" + std::to_string(address) + "]";
      outputCMPInstruction(ss, regOperand, memOperand.value);
      return 4;
    }

    outputCMPInstruction(ss, regOperand, memOperand.value);
    return 2;
  }

  case MODEncoding::MEMORY_MODE_8_DISPLACEMENT: {
    int8_t displacement = static_cast<int8_t>(bytestream[baseOffset + 2]);
    std::string baseAddress = getBaseRegisterAddress(rmBits);
    std::string memOperand = formatDisplacement(baseAddress, displacement);

    outputCMPInstruction(ss, regOperand, memOperand);
    return 3;
  }

  case MODEncoding::MEMORY_MODE_16_DISPLACEMENT: {
    int16_t displacement = readInt16(bytestream, baseOffset + 2);

    std::string baseAddress = getBaseRegisterAddress(rmBits);
    std::string memOperand = formatDisplacement(baseAddress, displacement);

    outputCMPInstruction(ss, regOperand, memOperand);
    return 4;
  }
  }

  throw std::runtime_error("Invalid addressing mode");
}

//-----------------------------------------------------------------------------
uint32_t CMPRegMemHandler::decode(std::stringstream &ss,
                                  const std::vector<char> &bytestream,
                                  uint32_t baseOffset) {
  // Opcode patterns:
  //   0x38 (0b00111000) mask 0b11111100: CMP reg/mem with register
  //
  // Byte format: [opcode][mod-reg-r/m][optional displacement]
  // D bit (bit 1): Direction - always 1 for CMP (comparison only in one
  // direction) W bit (bit 0): Word/Byte - 1 = word operation, 0 = byte
  // operation

  uint8_t opcode = static_cast<uint8_t>(bytestream[baseOffset]);

  bool isWordOperation = isBitSet(opcode, BIT_POS_W);

  uint8_t secondByte = static_cast<uint8_t>(bytestream[baseOffset + 1]);
  MODEncoding mod = getMODEncoding(secondByte);
  uint8_t regCode = (secondByte & BIT_FIELD_REG_MASK) >> 3;
  uint8_t rmCode = secondByte & BIT_FIELD_RM_MASK;

  return handleAddressingMode(ss, mod, rmCode, bytestream, baseOffset, regCode,
                              isWordOperation);
}
