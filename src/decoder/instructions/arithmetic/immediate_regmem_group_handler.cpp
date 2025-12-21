#include "immediate_regmem_group_handler.hpp"

#include "instructions/utilities/bit_utilities.hpp"
#include "instructions/utilities/instruction_output_utilities.hpp"
#include "arithmetic_immediate_regmem_handler.hpp"
#include <sstream>
#include <stdexcept>

//-----------------------------------------------------------------------------
uint32_t
ImmediateRegMemGroupHandler::decode(std::stringstream &ss,
                                    const std::vector<char> &bytestream,
                                    uint32_t baseOffset) {
  // The 0x80-0x83 group uses the REG field to distinguish instructions
  uint8_t secondByte = static_cast<uint8_t>(bytestream[baseOffset + 1]);
  uint8_t regCode = (secondByte & BIT_FIELD_REG_MASK) >> 3;

  switch (regCode) {
  case 0: {
    // ADD immediate to reg/mem
    auto addOutput = [](std::stringstream &ss, const std::string &dest,
                        const std::string &src) {
      outputInstruction(ss, "add", dest, src);
    };
    ArithmeticImmediateRegMemHandler handler("ADD (imm to reg/mem)", 0,
                                             addOutput);
    return handler.decode(ss, bytestream, baseOffset);
  }
  case 5: {
    // SUB immediate to reg/mem
    auto subOutput = [](std::stringstream &ss, const std::string &dest,
                        const std::string &src) {
      outputInstruction(ss, "sub", dest, src);
    };
    ArithmeticImmediateRegMemHandler handler("SUB (imm to reg/mem)", 5,
                                             subOutput);
    return handler.decode(ss, bytestream, baseOffset);
  }
  case 7: {
    // CMP immediate to reg/mem
    auto cmpOutput = [](std::stringstream &ss, const std::string &dest,
                        const std::string &src) {
      outputInstruction(ss, "cmp", dest, src);
    };
    ArithmeticImmediateRegMemHandler handler("CMP (imm to reg/mem)", 7,
                                             cmpOutput);
    return handler.decode(ss, bytestream, baseOffset);
  }
  default:
    throw std::runtime_error(
        "Invalid immediate to reg/mem instruction: REG field " +
        std::to_string(regCode) + " not supported (must be 0, 5, or 7)");
  }
}
