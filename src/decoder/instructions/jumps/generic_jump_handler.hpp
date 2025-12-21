#pragma once

#include "instructions/utilities/instruction_handler.hpp"
#include <string>
#include <string_view>

//-----------------------------------------------------------------------------
// Generic handler for jump instructions with 8-bit displacement
// Handles: All conditional jumps (JO, JE, JL, etc.) and loop instructions
class GenericJumpHandler : public InstructionHandler {
public:
  GenericJumpHandler(std::string_view mnemonic)
      : mnemonic_(mnemonic) {}

  uint32_t decode(std::stringstream &ss, const std::vector<char> &bytestream,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override { return mnemonic_; }

private:
  std::string_view mnemonic_;
};
