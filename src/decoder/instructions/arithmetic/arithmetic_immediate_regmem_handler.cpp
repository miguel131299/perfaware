#include "arithmetic_immediate_regmem_handler.hpp"

#include "instructions/mov/mov_utilities.hpp"
#include "instructions/utilities/bit_utilities.hpp"
#include "operand_decoder.hpp"
#include "tables/registers.hpp"
#include <sstream>
#include <stdexcept>
#include <string>

uint32_t ArithmeticImmediateRegMemHandler::handleAddressingMode(
    std::stringstream &ss, MODEncoding mod, uint8_t rmBits,
    const std::vector<char> &bytestream, uint32_t baseOffset,
    bool isWordOperation, bool isSignExtended) {

  switch (mod) {
  case MODEncoding::REGISTER_MODE: {
    std::string regOperand =
        std::string(getRegisterName(rmBits, isWordOperation));

    bool isImmediateWord = isWordOperation && !isSignExtended;
    Operand immediate = OperandDecoder::decodeImmediate(
        bytestream, baseOffset + 2, isImmediateWord);
    outputFunc_(ss, regOperand, immediate.value);
    return isImmediateWord ? 4 : 3;
  }

  case MODEncoding::MEMORY_MODE_NO_DISPLACEMENT: {
    Operand memOperand = OperandDecoder::decodeEffectiveAddress(
        rmBits, bytestream, baseOffset, false);

    if (rmBits == 6) {
      int16_t address = readInt16(bytestream, baseOffset + 2);
      memOperand.value = "[" + std::to_string(address) + "]";

      bool isImmediateWord = isWordOperation && !isSignExtended;
      Operand immediate = OperandDecoder::decodeImmediate(
          bytestream, baseOffset + 4, isImmediateWord);

      outputFunc_(ss,
                  addExplicitSizeToMemory(memOperand.value, isWordOperation),
                  immediate.value);

      return isImmediateWord ? 6 : 5;
    }

    bool isImmediateWord = isWordOperation && !isSignExtended;
    Operand immediate = OperandDecoder::decodeImmediate(
        bytestream, baseOffset + 2, isImmediateWord);

    outputFunc_(ss, addExplicitSizeToMemory(memOperand.value, isWordOperation),
                immediate.value);

    return isImmediateWord ? 4 : 3;
  }

  case MODEncoding::MEMORY_MODE_8_DISPLACEMENT: {
    int8_t displacement = static_cast<int8_t>(bytestream[baseOffset + 2]);
    std::string baseAddress = getBaseRegisterAddress(rmBits);
    std::string memOperand = formatDisplacement(baseAddress, displacement);

    bool isImmediateWord = isWordOperation && !isSignExtended;
    Operand immediate = OperandDecoder::decodeImmediate(
        bytestream, baseOffset + 3, isImmediateWord);

    outputFunc_(ss, addExplicitSizeToMemory(memOperand, isWordOperation),
                immediate.value);

    return isImmediateWord ? 5 : 4;
  }

  case MODEncoding::MEMORY_MODE_16_DISPLACEMENT: {
    int16_t displacement = readInt16(bytestream, baseOffset + 2);

    std::string baseAddress = getBaseRegisterAddress(rmBits);
    std::string memOperand = formatDisplacement(baseAddress, displacement);

    bool isImmediateWord = isWordOperation && !isSignExtended;
    Operand immediate = OperandDecoder::decodeImmediate(
        bytestream, baseOffset + 4, isImmediateWord);

    outputFunc_(ss, addExplicitSizeToMemory(memOperand, isWordOperation),
                immediate.value);
    return isImmediateWord ? 6 : 5;
  }
  }

  throw std::runtime_error("Invalid addressing mode");
}

//-----------------------------------------------------------------------------
uint32_t
ArithmeticImmediateRegMemHandler::decode(std::stringstream &ss,
                                         const std::vector<char> &bytestream,
                                         uint32_t baseOffset) {
  // Opcode patterns:
  //   0x80-0x83: Group of immediate operations
  //
  // Format: 100000sw
  // s = sign extension bit (bit 1)
  // w = word/byte bit (bit 0)
  // REG field (bits 5-3) determines the operation

  uint8_t opcode = static_cast<uint8_t>(bytestream[baseOffset]);
  bool isSignExtended = isBitSet(opcode, 1);
  bool isWordOperation = isBitSet(opcode, BIT_POS_W);

  uint8_t secondByte = static_cast<uint8_t>(bytestream[baseOffset + 1]);
  MODEncoding mod = getMODEncoding(secondByte);
  uint8_t regCode = (secondByte & BIT_FIELD_REG_MASK) >> 3;
  uint8_t rmCode = secondByte & BIT_FIELD_RM_MASK;

  if (regCode != expectedRegField_) {
    throw std::runtime_error("Invalid " + std::string(mnemonic_) +
                             " immediate to reg/mem: REG field must be " +
                             std::to_string(expectedRegField_) + ", got " +
                             std::to_string(regCode));
  }

  return handleAddressingMode(ss, mod, rmCode, bytestream, baseOffset,
                              isWordOperation, isSignExtended);
}
