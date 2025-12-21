#pragma once

#include "../instructions/instruction_handler.hpp"
#include "../instructions/mov_utilities.hpp"
#include <string_view>

//-----------------------------------------------------------------------------
// Handler for SUB reg/mem with register instruction (opcode pattern 0b00101100)
// Supports: register-to-register and register-to/from-memory addressing modes
class SUBRegMemHandler : public InstructionHandler {
public:
  uint32_t decode(std::stringstream &ss, const std::vector<char> &bytestream,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override {
    return "SUB (reg/mem)";
  }

private:
  // Helper to handle different addressing modes
  uint32_t handleAddressingMode(std::stringstream &ss, MODEncoding mod,
                                uint8_t rmBits,
                                const std::vector<char> &bytestream,
                                uint32_t baseOffset, uint8_t regCode,
                                bool isWordOperation, bool isDestReg);
};
