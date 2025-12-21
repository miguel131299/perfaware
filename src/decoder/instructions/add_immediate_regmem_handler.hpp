#pragma once

#include "../instructions/instruction_handler.hpp"
#include "../instructions/mov_utilities.hpp"
#include <string_view>

//-----------------------------------------------------------------------------
// Handler for ADD immediate to register/memory instruction (opcode pattern
// 0x80-0x83) Supports: immediate-to-register and immediate-to-memory addressing
// modes
class ADDImmediateRegMemHandler : public InstructionHandler {
public:
  uint32_t decode(std::stringstream &ss, const std::vector<char> &bytestream,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override {
    return "ADD (imm to reg/mem)";
  }

private:
  uint32_t handleAddressingMode(std::stringstream &ss, MODEncoding mod,
                                uint8_t rmBits,
                                const std::vector<char> &bytestream,
                                uint32_t baseOffset, bool isWordOperation,
                                bool isSignExtended);
};
