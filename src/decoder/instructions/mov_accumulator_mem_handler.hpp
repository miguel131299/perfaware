#pragma once

#include "../instructions/instruction_handler.hpp"
#include <string_view>

// Handler for MOV accumulator to memory instruction (opcode 0xA2/0xA3)
// Supports: accumulator-to-memory addressing mode (direct address only)
class MOVAccumulatorMemHandler : public InstructionHandler {
public:
  uint32_t decode(std::stringstream &ss, const std::vector<char> &bytestream,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override {
    return "MOV (accumulator to mem)";
  }
};
