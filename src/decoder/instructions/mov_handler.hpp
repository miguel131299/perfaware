#pragma once

#include "../instructions/instruction_handler.hpp"
#include <string_view>

// Handler for MOV instruction
// Supports: register-to-register, immediate-to-register, and memory addressing
// modes
class MOVHandler : public InstructionHandler {
public:
  uint32_t decode(std::stringstream &ss, const std::vector<char> &bytestream,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override { return "MOV"; }

private:
  // Handle register-to-register and register-to-memory MOV
  uint32_t handleRegMemToFromReg(std::stringstream &ss,
                                 const std::vector<char> &bytestream,
                                 uint32_t baseOffset);

  // Handle immediate-to-register MOV
  uint32_t handleImmediateToReg(std::stringstream &ss,
                                const std::vector<char> &bytestream,
                                uint32_t baseOffset);

  // Helper to handle different addressing modes
  uint32_t handleAddressingMode(std::stringstream &ss, uint8_t modBits,
                                uint8_t rmBits,
                                const std::vector<char> &bytestream,
                                uint32_t baseOffset, uint8_t regCode,
                                bool isWordOperation, bool isDestReg);
};
