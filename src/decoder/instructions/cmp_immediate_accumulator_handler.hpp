#pragma once

#include "../instructions/instruction_handler.hpp"
#include <string_view>

//-----------------------------------------------------------------------------
// Handler for CMP immediate to accumulator instruction (opcode pattern 0b00111100)
// Supports: immediate-to-AL/AX addressing mode
class CMPImmediateAccumulatorHandler : public InstructionHandler {
public:
  uint32_t decode(std::stringstream &ss, const std::vector<char> &bytestream,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override {
    return "CMP (imm to accumulator)";
  }
};
