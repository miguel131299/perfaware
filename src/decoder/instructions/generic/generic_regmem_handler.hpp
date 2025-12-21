#pragma once

#include "../utilities/instruction_handler.hpp"
#include "../mov/mov_utilities.hpp"
#include <functional>
#include <string>
#include <string_view>

//-----------------------------------------------------------------------------
// Generic handler for register/memory with register operations
// Handles: MOV, ADD, SUB, CMP - all with identical logic
// Works with any instruction that has D (direction) and W (word) bits
class GenericRegMemHandler : public InstructionHandler {
public:
  using OutputFunction = std::function<void(
      std::stringstream &, const std::string &, const std::string &)>;

  GenericRegMemHandler(std::string_view mnemonic, OutputFunction outputFunc)
      : mnemonic_(mnemonic), outputFunc_(outputFunc) {}

  uint32_t decode(std::stringstream &ss, const std::vector<char> &bytestream,
                  uint32_t baseOffset) override;

  [[nodiscard]] std::string_view getName() const override { return mnemonic_; }

private:
  std::string_view mnemonic_;
  OutputFunction outputFunc_;

  uint32_t handleAddressingMode(std::stringstream &ss, MODEncoding mod,
                                uint8_t rmBits,
                                const std::vector<char> &bytestream,
                                uint32_t baseOffset, uint8_t regCode,
                                bool isWordOperation, bool isDestReg);
};
