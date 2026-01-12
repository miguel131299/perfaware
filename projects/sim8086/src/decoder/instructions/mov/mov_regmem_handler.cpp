#include "mov_regmem_handler.hpp"

#include "instructions/utilities/bit_utilities.hpp"
#include "operand_decoder.hpp"
#include "tables/registers.hpp"
#include <sstream>
#include <stdexcept>
#include <string>

//-----------------------------------------------------------------------------
uint32_t MOVRegMemHandler::handleAddressingMode(
    std::stringstream &ss, MODEncoding mod, uint8_t rmBits,
    const std::vector<uint8_t> &bytes, uint32_t baseOffset, uint8_t regCode,
    bool isWordOperation, bool isDestReg) {
  std::string regOperand =
      std::string(getRegisterName(regCode, isWordOperation));

  switch (mod) {
  case MODEncoding::REGISTER_MODE: {
    // Both are registers
    std::string rmOperand =
        std::string(getRegisterName(rmBits, isWordOperation));
    if (isDestReg) {
      outputMOVInstruction(ss, regOperand, rmOperand);
    } else {
      outputMOVInstruction(ss, rmOperand, regOperand);
    }
    return 2;
  }

  case MODEncoding::MEMORY_MODE_NO_DISPLACEMENT: {
    Operand memOperand = OperandDecoder::decodeEffectiveAddress(
        rmBits, bytes, baseOffset, false);

    // Check for direct address mode (RM=6 with MOD=00)
    if (rmBits == 6) {
      // Direct address - read 2 more bytes
      int16_t address = readInt16(bytes, baseOffset + 2);
      memOperand.value = "[" + std::to_string(address) + "]";

      if (isDestReg) {
        outputMOVInstruction(ss, regOperand, memOperand.value);
      } else {
        outputMOVInstruction(ss, memOperand.value, regOperand);
      }
      return 4;
    }

    if (isDestReg) {
      outputMOVInstruction(ss, regOperand, memOperand.value);
    } else {
      outputMOVInstruction(ss, memOperand.value, regOperand);
    }
    return 2;
  }

  case MODEncoding::MEMORY_MODE_8_DISPLACEMENT: {
    int8_t displacement = static_cast<int8_t>(bytes[baseOffset + 2]);
    std::string baseAddress = getBaseRegisterAddress(rmBits);
    std::string memOperand = formatDisplacement(baseAddress, displacement);

    if (isDestReg) {
      outputMOVInstruction(ss, regOperand, memOperand);
    } else {
      outputMOVInstruction(ss, memOperand, regOperand);
    }
    return 3;
  }

  case MODEncoding::MEMORY_MODE_16_DISPLACEMENT: {
    int16_t displacement = readInt16(bytes, baseOffset + 2);

    std::string baseAddress = getBaseRegisterAddress(rmBits);
    std::string memOperand = formatDisplacement(baseAddress, displacement);

    if (isDestReg) {
      outputMOVInstruction(ss, regOperand, memOperand);
    } else {
      outputMOVInstruction(ss, memOperand, regOperand);
    }
    return 4;
  }
  }

  throw std::runtime_error("Invalid addressing mode");
}

//-----------------------------------------------------------------------------
uint32_t MOVRegMemHandler::decode(std::stringstream &ss,
                                  const std::vector<uint8_t> &bytes,
                                  uint32_t baseOffset) {
  // Word/Byte Operation
  bool isWordOperation = isBitSet(bytes[baseOffset], BIT_POS_W);

  // Direction: D=0 -> REG is source, D=1 -> REG is destination
  bool isRegDest = isBitSet(bytes[baseOffset], BIT_POS_D);

  uint8_t secondByte = static_cast<uint8_t>(bytes[baseOffset + 1]);
  MODEncoding mod = getMODEncoding(secondByte);
  uint8_t regCode = (secondByte & BIT_FIELD_REG_MASK) >> 3;
  uint8_t rmCode = secondByte & BIT_FIELD_RM_MASK;

  return handleAddressingMode(ss, mod, rmCode, bytes, baseOffset, regCode,
                              isWordOperation, isRegDest);
}
