#pragma once

#include "../instructions/instruction_handler.hpp"
#include <string_view>

// Handler for MOV memory to accumulator instruction (opcode 0xA0/0xA1)
// Supports: memory-to-AL/AX addressing mode (direct address only)
class MOVMemAccumulatorHandler : public InstructionHandler {
public:
  uint32_t decode(std::stringstream &ss, const std::vector<char> &bytestream,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override {
    return "MOV (mem to accumulator)";
  }
};
