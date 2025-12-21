#pragma once

#include "../utilities/instruction_handler.hpp"
#include <string_view>

//-----------------------------------------------------------------------------
// Dispatcher handler for 0x80-0x83 immediate to reg/mem group
// Routes to ADD, SUB, or CMP based on REG field (bits 5-3 of second byte)
// REG field meanings:
//   0 = ADD
//   5 = SUB
//   7 = CMP
class ImmediateRegMemGroupHandler : public InstructionHandler {
public:
  uint32_t decode(std::stringstream &ss, const std::vector<char> &bytestream,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override {
    return "Immediate to reg/mem (group)";
  }
};
