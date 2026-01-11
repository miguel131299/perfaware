#include "generic_jump_handler.hpp"

#include <cstdint>
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
uint32_t GenericJumpHandler::decode(std::stringstream &ss,
                                    const std::vector<char> &bytestream,
                                    uint32_t baseOffset) {
  // Jump instructions with 8-bit signed displacement
  // Format: [opcode][displacement]
  // Target address = baseOffset + 2 + sign_extended(displacement)

  int8_t displacement = static_cast<int8_t>(bytestream[baseOffset + 1]);

  // Calculate target address (relative to next instruction)
  int32_t targetOffset = static_cast<int32_t>(baseOffset) + 2 + displacement;

  // Output in the format "JNZ $-6" where $ is the current instruction
  // displacement from current instruction to target
  int32_t offsetFromCurrent = targetOffset - static_cast<int32_t>(baseOffset);

  ss << mnemonic_ << " $";
  if (offsetFromCurrent >= 0) {
    ss << "+";
  }
  ss << offsetFromCurrent << "\n";

  return 2; // opcode + displacement
}
