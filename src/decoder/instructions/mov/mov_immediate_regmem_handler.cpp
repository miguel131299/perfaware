#include "mov_immediate_regmem_handler.hpp"

#include "instructions/utilities/bit_utilities.hpp"
#include "mov_regmem_handler.hpp"
#include "mov_utilities.hpp"
#include "operand_decoder.hpp"
#include "tables/registers.hpp"
#include <sstream>
#include <stdexcept>
#include <string>

uint32_t MOVImmediateRegMemHandler::handleAddressingMode(
    std::stringstream &ss, MODEncoding mod, uint8_t rmBits,
    const std::vector<char> &bytestream, uint32_t baseOffset,
    bool isWordOperation) {

  switch (mod) {
  case MODEncoding::REGISTER_MODE: {
    // In this case, regCode is always 0 -> AX/AL
    std::string regOperand = std::string(getRegisterName(0, isWordOperation));
    Operand immediate = OperandDecoder::decodeImmediate(
        bytestream, baseOffset + 2, isWordOperation);
    outputMOVInstruction(ss, regOperand, immediate.value);
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

      Operand immediate = OperandDecoder::decodeImmediate(
          bytestream, baseOffset + 4, isWordOperation);

      outputMOVInstruction(
          ss, addExplicitSizeToMemory(memOperand.value, isWordOperation),
          immediate.value);

      return isWordOperation ? 6 : 5;
    }

    Operand immediate = OperandDecoder::decodeImmediate(
        bytestream, baseOffset + 2, isWordOperation);

    outputMOVInstruction(
        ss, addExplicitSizeToMemory(memOperand.value, isWordOperation),
        immediate.value);

    return isWordOperation ? 4 : 3;
  }

  case MODEncoding::MEMORY_MODE_8_DISPLACEMENT: {
    int8_t displacement = static_cast<int8_t>(bytestream[baseOffset + 2]);
    std::string baseAddress = getBaseRegisterAddress(rmBits);
    std::string memOperand = formatDisplacement(baseAddress, displacement);

    Operand immediate = OperandDecoder::decodeImmediate(
        bytestream, baseOffset + (isWordOperation ? 4 : 3), isWordOperation);

    outputMOVInstruction(ss,
                         addExplicitSizeToMemory(memOperand, isWordOperation),
                         immediate.value);

    return isWordOperation ? 5 : 4;
  }

  case MODEncoding::MEMORY_MODE_16_DISPLACEMENT: {
    int16_t displacement = readInt16(bytestream, baseOffset + 2);

    std::string baseAddress = getBaseRegisterAddress(rmBits);
    std::string memOperand = formatDisplacement(baseAddress, displacement);

    Operand immediate = OperandDecoder::decodeImmediate(
        bytestream, baseOffset + (isWordOperation ? 4 : 3), isWordOperation);

    outputMOVInstruction(ss,
                         addExplicitSizeToMemory(memOperand, isWordOperation),
                         immediate.value);
    return isWordOperation ? 6 : 5;
  }
  }

  throw std::runtime_error("Invalid addressing mode");
}

//-----------------------------------------------------------------------------
uint32_t MOVImmediateRegMemHandler::decode(std::stringstream &ss,
                                           const std::vector<char> &bytestream,
                                           uint32_t baseOffset) {
  // Opcode patterns:
  //   0xC6 (0b11000110) mask 0b11111110: MOV immediate to reg/mem (byte)
  //   0xC7 (0b11000111) mask 0b11111110: MOV immediate to reg/mem (word)
  //
  // Byte format: [opcode][mod-reg-r/m][immediate...][displacement...]
  // The REG field (bits 5-3 of mod-reg-r/m) should be 0 for MOV

  // Steps:
  // 1. Extract W bit from opcode to determine word (1) or byte (0) operation
  // Word/Byte Operation
  bool isWordOperation = isBitSet(bytestream[baseOffset], BIT_POS_W);

  // 2. Extract MOD, REG, and R/M from second byte
  uint8_t secondByte = static_cast<uint8_t>(bytestream[baseOffset + 1]);
  MODEncoding mod = getMODEncoding(secondByte);
  uint8_t regCode = (secondByte & BIT_FIELD_REG_MASK) >> 3;
  uint8_t rmCode = secondByte & BIT_FIELD_RM_MASK;

  // 3. Verify REG field is 0 (otherwise it's not a MOV immediate)
  if (regCode != 0) {
    throw std::runtime_error(
        "Invalid MOV immediate to reg/mem: REG field must be 0, got " +
        std::to_string(regCode));
  }

  // 4. Based on MOD, handle different addressing modes (similar to
  // MOVRegMemHandler)
  // 5. Decode the immediate value
  // 6. Output the instruction
  // 7. Return bytes consumed
  return handleAddressingMode(ss, mod, rmCode, bytestream, baseOffset,
                              isWordOperation);
}
