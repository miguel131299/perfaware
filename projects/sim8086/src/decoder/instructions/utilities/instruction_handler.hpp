#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>

// Abstract base class for instruction handlers
// Each instruction type (MOV, ADD, SUB, etc.) gets its own handler
class InstructionHandler {
public:
  virtual ~InstructionHandler() = default;

  // Decode and output the instruction
  // Returns the number of bytes consumed
  virtual uint32_t decode(std::stringstream &ss,
                          const std::vector<uint8_t> &bytes,
                          uint32_t baseOffset) = 0;

  // Get the name of this instruction
  [[nodiscard]] virtual std::string_view getName() const = 0;
};
