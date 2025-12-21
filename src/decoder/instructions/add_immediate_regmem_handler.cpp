#include "add_immediate_regmem_handler.hpp"

#include "../bit_utilities.hpp"
#include "../operand_decoder.hpp"
#include "../tables/registers.hpp"
#include "add_utilities.hpp"
#include <sstream>
#include <stdexcept>
#include <string>

static std::string addExplicitSizeToImmediate(std::string &immediate,
                                              bool isWordOperation) {
  return std::string(isWordOperation ? "word" : "byte") + " " + immediate;
}

uint32_t ADDImmediateRegMemHandler::handleAddressingMode(
    std::stringstream &ss, MODEncoding mod, uint8_t rmBits,
    const std::vector<char> &bytestream, uint32_t baseOffset,
    bool isWordOperation, bool isSignExtended) {

  switch (mod) {
  case MODEncoding::REGISTER_MODE: {
    // In this case, regCode is always 0 (encoded in opcode pattern)
    std::string regOperand =
        std::string(getRegisterName(rmBits, isWordOperation));

    // If sign-extended, immediate is 1 byte; otherwise 2 bytes if word
    // operation
    bool isImmediateWord = isWordOperation && !isSignExtended;
    Operand immediate = OperandDecoder::decodeImmediate(
        bytestream, baseOffset + 2, isImmediateWord);
    outputADDInstruction(ss, regOperand, immediate.value);
    return isImmediateWord ? 4 : 3;
  }

  case MODEncoding::MEMORY_MODE_NO_DISPLACEMENT: {
    Operand memOperand = OperandDecoder::decodeEffectiveAddress(
        rmBits, bytestream, baseOffset, false);

    // Check for direct address mode (RM=6 with MOD=00)
    if (rmBits == 6) {
      // Direct address - read 2 more bytes
      int16_t address = readInt16(bytestream, baseOffset + 2);
      memOperand.value = "[" + std::to_string(address) + "]";

      bool isImmediateWord = isWordOperation && !isSignExtended;
      Operand immediate = OperandDecoder::decodeImmediate(
          bytestream, baseOffset + 4, isImmediateWord);

      outputADDInstruction(ss, memOperand.value, immediate.value);

      return isImmediateWord ? 6 : 5;
    }

    bool isImmediateWord = isWordOperation && !isSignExtended;
    Operand immediate = OperandDecoder::decodeImmediate(
        bytestream, baseOffset + 2, isImmediateWord);

    outputADDInstruction(
        ss, memOperand.value,
        addExplicitSizeToImmediate(immediate.value, isWordOperation));

    return isImmediateWord ? 4 : 3;
  }

  case MODEncoding::MEMORY_MODE_8_DISPLACEMENT: {
    int8_t displacement = static_cast<int8_t>(bytestream[baseOffset + 2]);
    std::string baseAddress = getBaseRegisterAddress(rmBits);
    std::string memOperand = formatDisplacement(baseAddress, displacement);

    bool isImmediateWord = isWordOperation && !isSignExtended;
    // Immediate is at baseOffset + 3 (opcode + mod-reg-r/m + 1-byte
    // displacement)
    Operand immediate = OperandDecoder::decodeImmediate(
        bytestream, baseOffset + 3, isImmediateWord);

    outputADDInstruction(
        ss, memOperand,
        addExplicitSizeToImmediate(immediate.value, isWordOperation));

    return isImmediateWord ? 5 : 4;
  }

  case MODEncoding::MEMORY_MODE_16_DISPLACEMENT: {
    int16_t displacement = readInt16(bytestream, baseOffset + 2);

    std::string baseAddress = getBaseRegisterAddress(rmBits);
    std::string memOperand = formatDisplacement(baseAddress, displacement);

    bool isImmediateWord = isWordOperation && !isSignExtended;
    // Immediate is at baseOffset + 4 (opcode + mod-reg-r/m + 2-byte
    // displacement)
    Operand immediate = OperandDecoder::decodeImmediate(
        bytestream, baseOffset + 4, isImmediateWord);

    outputADDInstruction(
        ss, memOperand,
        addExplicitSizeToImmediate(immediate.value, isWordOperation));
    return isImmediateWord ? 6 : 5;
  }
  }

  throw std::runtime_error("Invalid addressing mode");
}

//-----------------------------------------------------------------------------
uint32_t ADDImmediateRegMemHandler::decode(std::stringstream &ss,
                                           const std::vector<char> &bytestream,
                                           uint32_t baseOffset) {
  // Opcode patterns:
  //   0x80 (0b10000000) mask 0b11111100: ADD immediate to reg/mem
  //   0x81 (0b10000001) mask 0b11111100: ADD immediate to reg/mem
  //   0x82 (0b10000010) mask 0b11111100: ADD immediate to reg/mem
  //   0x83 (0b10000011) mask 0b11111100: ADD immediate to reg/mem
  //
  // Format: 100000sw
  // s = sign extension bit (bit 1): if 1, 8-bit immediate is sign-extended
  // w = word/byte bit (bit 0): 1 = word, 0 = byte
  //
  // Immediate size rules:
  // - If s=1: immediate is 1 byte (sign-extended)
  // - If s=0 and w=1: immediate is 2 bytes (word)
  // - If s=0 and w=0: immediate is 1 byte (byte)
  //
  // Byte format: [opcode][mod-reg-r/m][displacement...][immediate...]
  // The REG field (bits 5-3 of mod-reg-r/m) must be 0 for ADD

  uint8_t opcode = static_cast<uint8_t>(bytestream[baseOffset]);
  bool isSignExtended = isBitSet(opcode, 1); // s bit at position 1
  bool isWordOperation = isBitSet(opcode, BIT_POS_W);

  uint8_t secondByte = static_cast<uint8_t>(bytestream[baseOffset + 1]);
  MODEncoding mod = getMODEncoding(secondByte);
  uint8_t regCode = (secondByte & BIT_FIELD_REG_MASK) >> 3;
  uint8_t rmCode = secondByte & BIT_FIELD_RM_MASK;

  // Verify REG field is 0 (otherwise it's not an ADD immediate)
  if (regCode != 0) {
    throw std::runtime_error(
        "Invalid ADD immediate to reg/mem: REG field must be 0, got " +
        std::to_string(regCode));
  }

  return handleAddressingMode(ss, mod, rmCode, bytestream, baseOffset,
                              isWordOperation, isSignExtended);
}
