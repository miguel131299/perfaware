#pragma once

#include "instructions/utilities/instruction_handler.hpp"
#include "instructions/mov/mov_utilities.hpp"
#include <string_view>

// Handler for MOV immediate to register/memory instruction (opcode pattern
// 0xC6/0xC7) Supports: immediate-to-register and immediate-to-memory addressing
// modes
class MOVImmediateRegMemHandler : public InstructionHandler {
public:
  uint32_t decode(std::stringstream &ss, const std::vector<uint8_t> &bytes,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override {
    return "MOV (imm to reg/mem)";
  }

private:
  uint32_t handleAddressingMode(std::stringstream &ss, MODEncoding mod,
                                uint8_t rmBits,
                                const std::vector<uint8_t> &bytes,
                                uint32_t baseOffset, bool isWordOperation);
};
