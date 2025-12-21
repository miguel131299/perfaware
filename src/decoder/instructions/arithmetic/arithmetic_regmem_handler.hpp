#pragma once

#include "instructions/utilities/instruction_handler.hpp"
#include "instructions/mov/mov_utilities.hpp"
#include <functional>
#include <string_view>

//-----------------------------------------------------------------------------
// Generic handler for reg/mem with register arithmetic operations
// Handles: ADD, SUB with identical logic, parameterized by output function
class ArithmeticRegMemHandler : public InstructionHandler {
public:
  using OutputFunction = std::function<void(std::stringstream &, const std::string &, const std::string &)>;

  ArithmeticRegMemHandler(std::string_view mnemonic, OutputFunction outputFunc)
      : mnemonic_(mnemonic), outputFunc_(outputFunc) {}

  uint32_t decode(std::stringstream &ss, const std::vector<char> &bytestream,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override { return mnemonic_; }

private:
  uint32_t handleAddressingMode(std::stringstream &ss, MODEncoding mod,
                                uint8_t rmBits,
                                const std::vector<char> &bytestream,
                                uint32_t baseOffset, uint8_t regCode,
                                bool isWordOperation, bool isDestReg);

  std::string_view mnemonic_;
  OutputFunction outputFunc_;
};
