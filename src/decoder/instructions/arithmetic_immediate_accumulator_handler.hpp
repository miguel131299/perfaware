#pragma once

#include "../instructions/instruction_handler.hpp"
#include <functional>
#include <string_view>

//-----------------------------------------------------------------------------
// Generic handler for immediate to accumulator arithmetic operations
// Handles: ADD, SUB, CMP with identical logic, parameterized by output function
class ArithmeticImmediateAccumulatorHandler : public InstructionHandler {
public:
  using OutputFunction = std::function<void(std::stringstream &, const std::string &, const std::string &)>;

  ArithmeticImmediateAccumulatorHandler(std::string_view mnemonic, OutputFunction outputFunc)
      : mnemonic_(mnemonic), outputFunc_(outputFunc) {}

  uint32_t decode(std::stringstream &ss, const std::vector<char> &bytestream,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override { return mnemonic_; }

private:
  std::string_view mnemonic_;
  OutputFunction outputFunc_;
};
