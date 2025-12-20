#include "mov_handler.hpp"

#include "../bit_utilities.hpp"
#include "../operand_decoder.hpp"
#include "../tables/registers.hpp"
#include <bitset>
#include <format>
#include <sstream>
#include <stdexcept>
#include <string>

//-----------------------------------------------------------------------------
enum class MODEncoding : uint8_t {
  MEMORY_MODE_NO_DISPLACEMENT,
  MEMORY_MODE_8_DISPLACEMENT,
  MEMORY_MODE_16_DISPLACEMENT,
  REGISTER_MODE,
};

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
// Helper to format and output MOV instruction
static void outputMOVInstruction(std::stringstream &ss, const std::string &dest,
                                 const std::string &src) {
  ss << std::format("mov {}, {}\n", dest, src);
}

//-----------------------------------------------------------------------------
// Helper to build base register address string from RM field
static std::string getBaseRegisterAddress(uint8_t rmBits) {
  switch (rmBits) {
    case 0: return "BX + SI";
    case 1: return "BX + DI";
    case 2: return "BP + SI";
    case 3: return "BP + DI";
    case 4: return "SI";
    case 5: return "DI";
    case 6: return "BP";
    case 7: return "BX";
    default: return "";
  }
}

//-----------------------------------------------------------------------------
uint32_t MOVHandler::handleRegMemToFromReg(std::stringstream &ss,
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

  return handleAddressingMode(ss, static_cast<uint8_t>(mod), rmCode, bytestream,
                              baseOffset, regCode, isWordOperation, isRegDest);
}

//-----------------------------------------------------------------------------
uint32_t MOVHandler::handleImmediateToReg(std::stringstream &ss,
                                          const std::vector<char> &bytestream,
                                          uint32_t baseOffset) {
  // Bit 3 indicates word (1) or byte (0) operation
  bool isWordOperation = isBitSet(bytestream[baseOffset], 3);

  // Register code is in bits 2-0
  uint8_t regCode = bytestream[baseOffset] & MOV_IMM_REG_MASK;
  std::string destReg = std::string(getRegisterName(regCode, isWordOperation));

  Operand immediate = OperandDecoder::decodeImmediate(
      bytestream, baseOffset + 1, isWordOperation);

  outputMOVInstruction(ss, destReg, immediate.value);

  return isWordOperation ? 3 : 2;
}

//-----------------------------------------------------------------------------
uint32_t MOVHandler::handleAddressingMode(std::stringstream &ss,
                                          uint8_t modBits, uint8_t rmBits,
                                          const std::vector<char> &bytestream,
                                          uint32_t baseOffset, uint8_t regCode,
                                          bool isWordOperation,
                                          bool isDestReg) {

  MODEncoding mod = static_cast<MODEncoding>(modBits);
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
    std::string base = "[" + getBaseRegisterAddress(rmBits);

    if (displacement != 0) {
      base += " + " + std::to_string(displacement);
    }
    base += "]";

    if (isDestReg) {
      outputMOVInstruction(ss, regOperand, base);
    } else {
      outputMOVInstruction(ss, base, regOperand);
    }
    return 3;
  }

  case MODEncoding::MEMORY_MODE_16_DISPLACEMENT: {
    uint8_t lowByte = static_cast<uint8_t>(bytestream[baseOffset + 2]);
    uint8_t highByte = static_cast<uint8_t>(bytestream[baseOffset + 3]);
    int16_t displacement = lowByte | (static_cast<int16_t>(highByte) << 8);

    std::string base = "[" + getBaseRegisterAddress(rmBits);

    if (displacement != 0) {
      base += " + " + std::to_string(displacement);
    }
    base += "]";

    if (isDestReg) {
      outputMOVInstruction(ss, regOperand, base);
    } else {
      outputMOVInstruction(ss, base, regOperand);
    }
    return 4;
  }
  }

  throw std::runtime_error("Invalid addressing mode");
}

//-----------------------------------------------------------------------------
uint32_t MOVHandler::decode(std::stringstream &ss,
                            const std::vector<char> &bytestream,
                            uint32_t baseOffset) {
  uint8_t opcode = static_cast<uint8_t>(bytestream[baseOffset]);

  // Register/memory to/from register
  if ((opcode & 0b11111100) == 0b10001000) {
    return handleRegMemToFromReg(ss, bytestream, baseOffset);
  }

  // Immediate to register
  if ((opcode & 0b11110000) == 0b10110000) {
    return handleImmediateToReg(ss, bytestream, baseOffset);
  }

  throw std::runtime_error(
      "Invalid MOV instruction opcode: " + std::to_string(opcode) + " (0b" +
      std::bitset<8>(opcode).to_string() + ")");
}
