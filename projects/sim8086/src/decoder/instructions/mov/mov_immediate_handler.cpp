#include "mov_immediate_handler.hpp"

#include "instructions/utilities/bit_utilities.hpp"
#include "operand_decoder.hpp"
#include "tables/registers.hpp"
#include "mov_utilities.hpp"
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
uint32_t MOVImmediateHandler::decode(std::stringstream &ss,
                                     const std::vector<uint8_t> &bytes,
                                     uint32_t baseOffset) {
  // Bit 3 indicates word (1) or byte (0) operation
  bool isWordOperation = isBitSet(bytes[baseOffset], 3);

  // Register code is in bits 2-0
  uint8_t regCode = bytes[baseOffset] & MOV_IMM_REG_MASK;
  std::string destReg = std::string(getRegisterName(regCode, isWordOperation));

  Operand immediate = OperandDecoder::decodeImmediate(
      bytes, baseOffset + 1, isWordOperation);

  outputMOVInstruction(ss, destReg, immediate.value);

  return isWordOperation ? 3 : 2;
}
