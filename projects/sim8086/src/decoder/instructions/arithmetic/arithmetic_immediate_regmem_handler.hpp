#pragma once

#include "instructions/utilities/instruction_handler.hpp"
#include "instructions/mov/mov_utilities.hpp"
#include <functional>
#include <string>
#include <string_view>

//-----------------------------------------------------------------------------
// Generic handler for immediate to register/memory arithmetic operations
// Handles: ADD, SUB, CMP with identical logic, parameterized by mnemonic
class ArithmeticImmediateRegMemHandler : public InstructionHandler {
public:
  using OutputFunction = std::function<void(std::stringstream &, const std::string &, const std::string &)>;

  ArithmeticImmediateRegMemHandler(std::string_view mnemonic, uint8_t expectedRegField, OutputFunction outputFunc)
      : mnemonic_(mnemonic), expectedRegField_(expectedRegField), outputFunc_(outputFunc) {}

  uint32_t decode(std::stringstream &ss, const std::vector<uint8_t> &bytes,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override { return mnemonic_; }

private:
  uint32_t handleAddressingMode(std::stringstream &ss, MODEncoding mod,
                                uint8_t rmBits,
                                const std::vector<uint8_t> &bytes,
                                uint32_t baseOffset, bool isWordOperation,
                                bool isSignExtended);

  std::string_view mnemonic_;
  uint8_t expectedRegField_;
  OutputFunction outputFunc_;
};
