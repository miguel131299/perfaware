#include "add_regmem_handler.hpp"

#include "../bit_utilities.hpp"
#include "../operand_decoder.hpp"
#include "../tables/registers.hpp"
#include "add_utilities.hpp"
#include <format>
#include <sstream>
#include <stdexcept>
#include <string>

//-----------------------------------------------------------------------------
uint32_t ADDRegMemHandler::handleAddressingMode(
    std::stringstream &ss, MODEncoding mod, uint8_t rmBits,
    const std::vector<char> &bytestream, uint32_t baseOffset, uint8_t regCode,
    bool isWordOperation, bool isDestReg) {
  std::string regOperand =
      std::string(getRegisterName(regCode, isWordOperation));

  switch (mod) {
  case MODEncoding::REGISTER_MODE: {
    // Both are registers
    std::string rmOperand =
        std::string(getRegisterName(rmBits, isWordOperation));
    if (isDestReg) {
      outputADDInstruction(ss, regOperand, rmOperand);
    } else {
      outputADDInstruction(ss, rmOperand, regOperand);
    }
    return 2;
  }

  case MODEncoding::MEMORY_MODE_NO_DISPLACEMENT: {
    Operand memOperand = OperandDecoder::decodeEffectiveAddress(
        rmBits, bytestream, baseOffset, false);

    // Check for direct address mode (RM=6 with MOD=00)
    if (rmBits == 6) {
      // Direct address - read 2 more bytes
      int16_t address = readInt16(bytestream, baseOffset + 2);
      memOperand.value = "[" + std::to_string(address) + "]";

      if (isDestReg) {
        outputADDInstruction(ss, regOperand, memOperand.value);
      } else {
        outputADDInstruction(ss, memOperand.value, regOperand);
      }

      return 4;
    }

    if (isDestReg) {
      outputADDInstruction(ss, regOperand, memOperand.value);
    } else {
      outputADDInstruction(ss, memOperand.value, regOperand);
    }

    return 2;
  }

  case MODEncoding::MEMORY_MODE_8_DISPLACEMENT: {
    int8_t displacement = static_cast<int8_t>(bytestream[baseOffset + 2]);
    std::string baseAddress = getBaseRegisterAddress(rmBits);
    std::string memOperand = formatDisplacement(baseAddress, displacement);

    if (isDestReg) {
      outputADDInstruction(ss, regOperand, memOperand);
    } else {
      outputADDInstruction(ss, memOperand, regOperand);
    }

    return 3;
  }

  case MODEncoding::MEMORY_MODE_16_DISPLACEMENT: {
    int16_t displacement = readInt16(bytestream, baseOffset + 2);

    std::string baseAddress = getBaseRegisterAddress(rmBits);
    std::string memOperand = formatDisplacement(baseAddress, displacement);

    if (isDestReg) {
      outputADDInstruction(ss, regOperand, memOperand);
    } else {
      outputADDInstruction(ss, memOperand, regOperand);
    }

    return 4;
  }
  }

  throw std::runtime_error("Invalid addressing mode");
}

//-----------------------------------------------------------------------------
uint32_t ADDRegMemHandler::decode(std::stringstream &ss,
                                  const std::vector<char> &bytestream,
                                  uint32_t baseOffset) {
  // Opcode patterns:
  //   0x00 (0b00000000) mask 0b11111100: ADD reg/mem with register
  //
  // Byte format: [opcode][mod-reg-r/m][optional displacement]
  // D bit (bit 1): Direction - 1 = reg is destination, 0 = reg is source
  // W bit (bit 0): Word/Byte - 1 = word operation, 0 = byte operation

  uint8_t opcode = static_cast<uint8_t>(bytestream[baseOffset]);

  bool isDestReg = isBitSet(opcode, BIT_POS_D);
  bool isWordOperation = isBitSet(opcode, BIT_POS_W);

  uint8_t secondByte = static_cast<uint8_t>(bytestream[baseOffset + 1]);
  MODEncoding mod = getMODEncoding(secondByte);
  uint8_t regCode = (secondByte & BIT_FIELD_REG_MASK) >> 3;
  uint8_t rmCode = secondByte & BIT_FIELD_RM_MASK;

  return handleAddressingMode(ss, mod, rmCode, bytestream, baseOffset,
                              regCode, isWordOperation, isDestReg);
}
