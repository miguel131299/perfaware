#pragma once

#include "../utilities/instruction_handler.hpp"
#include "../mov/mov_utilities.hpp"
#include <string_view>

//-----------------------------------------------------------------------------
// Handler for MOV reg/mem to/from reg instruction (opcode pattern 0b10001000)
// Supports: register-to-register and register-to/from-memory addressing modes
class MOVRegMemHandler : public InstructionHandler {
public:
  uint32_t decode(std::stringstream &ss, const std::vector<char> &bytestream,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override {
    return "MOV (reg/mem)";
  }

private:
  // Helper to handle different addressing modes
  uint32_t handleAddressingMode(std::stringstream &ss, MODEncoding mod,
                                uint8_t rmBits,
                                const std::vector<char> &bytestream,
                                uint32_t baseOffset, uint8_t regCode,
                                bool isWordOperation, bool isDestReg);
};
