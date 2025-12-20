#include "mov_regmem_handler.hpp"

#include "../bit_utilities.hpp"
#include "../operand_decoder.hpp"
#include "../tables/registers.hpp"
#include <format>
#include <sstream>
#include <stdexcept>
#include <string>

//-----------------------------------------------------------------------------
static MODEncoding getMODEncoding(uint8_t byte) {
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
static bool isBitSet(uint8_t byte, size_t bitPosition) {
  return (byte & (1 << bitPosition)) != 0;
}

//-----------------------------------------------------------------------------
static void outputMOVInstruction(std::stringstream &ss, const std::string &dest,
                                 const std::string &src) {
  ss << std::format("mov {}, {}\n", dest, src);
}

//-----------------------------------------------------------------------------
static std::string getBaseRegisterAddress(uint8_t rmBits) {
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
static std::string formatDisplacement(const std::string &baseAddress,
                                      int16_t displacement) {
  if (displacement == 0) {
    return "[" + baseAddress + "]";
  }
  
  if (displacement < 0) {
    return "[" + baseAddress + " - " + std::to_string(-displacement) + "]";
  }
  
  return "[" + baseAddress + " + " + std::to_string(displacement) + "]";
}

//-----------------------------------------------------------------------------
uint32_t MOVRegMemHandler::handleAddressingMode(
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
      outputMOVInstruction(ss, regOperand, rmOperand);
    } else {
      outputMOVInstruction(ss, rmOperand, regOperand);
    }
    return 2;
  }

  case MODEncoding::MEMORY_MODE_NO_DISPLACEMENT: {
    Operand memOperand = OperandDecoder::decodeEffectiveAddress(
        rmBits, bytestream, baseOffset, false);

    // Check for direct address mode (RM=6 with MOD=00)
    if (rmBits == 6) {
      // Direct address - read 2 more bytes
      uint8_t lowByte = static_cast<uint8_t>(bytestream[baseOffset + 2]);
      uint8_t highByte = static_cast<uint8_t>(bytestream[baseOffset + 3]);
      int16_t address = lowByte | (static_cast<int16_t>(highByte) << 8);
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
    int8_t displacement = static_cast<int8_t>(bytestream[baseOffset + 2]);
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
    uint8_t lowByte = static_cast<uint8_t>(bytestream[baseOffset + 2]);
    uint8_t highByte = static_cast<uint8_t>(bytestream[baseOffset + 3]);
    int16_t displacement = lowByte | (static_cast<int16_t>(highByte) << 8);

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
                                  const std::vector<char> &bytestream,
                                  uint32_t baseOffset) {
  // Word/Byte Operation
  bool isWordOperation = isBitSet(bytestream[baseOffset], BIT_POS_W);

  // Direction: D=0 -> REG is source, D=1 -> REG is destination
  bool isRegDest = isBitSet(bytestream[baseOffset], BIT_POS_D);

  uint8_t secondByte = static_cast<uint8_t>(bytestream[baseOffset + 1]);
  MODEncoding mod = getMODEncoding(secondByte);
  uint8_t regCode = (secondByte & BIT_FIELD_REG_MASK) >> 3;
  uint8_t rmCode = secondByte & BIT_FIELD_RM_MASK;

  return handleAddressingMode(ss, mod, rmCode, bytestream, baseOffset, regCode,
                              isWordOperation, isRegDest);
}
