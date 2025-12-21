#include "immediate_regmem_group_handler.hpp"

#include "../bit_utilities.hpp"
#include "add_immediate_regmem_handler.hpp"
#include "cmp_immediate_regmem_handler.hpp"
#include "sub_immediate_regmem_handler.hpp"
#include <memory>
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
    ADDImmediateRegMemHandler handler;
    return handler.decode(ss, bytestream, baseOffset);
  }
  case 5: {
    // SUB immediate to reg/mem
    SUBImmediateRegMemHandler handler;
    return handler.decode(ss, bytestream, baseOffset);
  }
  case 7: {
    // CMP immediate to reg/mem
    CMPImmediateRegMemHandler handler;
    return handler.decode(ss, bytestream, baseOffset);
  }
  default:
    throw std::runtime_error(
        "Invalid immediate to reg/mem instruction: REG field " +
        std::to_string(regCode) + " not supported (must be 0, 5, or 7)");
  }
}
