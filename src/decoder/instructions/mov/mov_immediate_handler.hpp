#pragma once

#include "instructions/utilities/instruction_handler.hpp"
#include <string_view>

// Handler for MOV immediate to register instruction (opcode pattern 0b10110000)
class MOVImmediateHandler : public InstructionHandler {
 public:
  uint32_t decode(std::stringstream &ss, const std::vector<uint8_t> &bytes,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override {
    return "MOV (immediate)";
  }
};
