#include "add_immediate_accumulator_handler.hpp"

#include "../bit_utilities.hpp"
#include "../operand_decoder.hpp"
#include "../tables/registers.hpp"
#include "add_utilities.hpp"
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
uint32_t
ADDImmediateAccumulatorHandler::decode(std::stringstream &ss,
                                       const std::vector<char> &bytestream,
                                       uint32_t baseOffset) {
  // Opcode patterns:
  //   0x04 (0b00000100): ADD AL, imm8
  //   0x05 (0b00000101): ADD AX, imm16
  //
  // Byte format: [opcode][immediate...]
  // W bit (bit 0): Word (1) or byte (0) operation

  bool isWordOperation = isBitSet(bytestream[baseOffset], BIT_POS_W);

  std::string destReg =
      std::string(getRegisterName(0, isWordOperation)); // 0 = AX/AL

  Operand immediate = OperandDecoder::decodeImmediate(
      bytestream, baseOffset + 1, isWordOperation);

  outputADDInstruction(ss, destReg, immediate.value);

  return isWordOperation ? 3 : 2;
}
