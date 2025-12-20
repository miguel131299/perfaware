#include <bitset>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <sim8086/decoder.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

#include "instructions/instruction_handler.hpp"
#include "instructions/mov_immediate_handler.hpp"
#include "instructions/mov_immediate_regmem_handler.hpp"
#include "instructions/mov_regmem_handler.hpp"

//-----------------------------------------------------------------------------
// Create instruction handler registry
// Maps instruction patterns to their handlers
static std::map<uint8_t,
                std::pair<uint8_t, std::unique_ptr<InstructionHandler>>>
createInstructionRegistry() {
  std::map<uint8_t, std::pair<uint8_t, std::unique_ptr<InstructionHandler>>>
      registry;

  // MOV instructions (each pattern gets its own handler)
  auto movRegMemHandler = std::make_unique<MOVRegMemHandler>();
  registry[0b10001000] = {0b11111100, std::move(movRegMemHandler)};

  auto movImmediateHandler = std::make_unique<MOVImmediateHandler>();
  registry[0b10110000] = {0b11110000, std::move(movImmediateHandler)};

  auto movImmediateRegMemHandler =
      std::make_unique<MOVImmediateRegMemHandler>();
  registry[0b11000110] = {0b11111110, std::move(movImmediateRegMemHandler)};

  // TODO: Add more instruction types here
  // auto addHandler = std::make_unique<ADDHandler>();
  // registry[0b00000100] = {0b11111110, std::move(addHandler)};

  return registry;
}

//-----------------------------------------------------------------------------
// Print the instruction that was just decoded to stderr for debugging
static void printDecodedInstruction(const std::stringstream &ss, size_t posBefore, size_t posAfter) {
  std::string instruction = ss.str().substr(posBefore, posAfter - posBefore);
  if (!instruction.empty()) {
    std::cerr << instruction;
  }
}

//-----------------------------------------------------------------------------
std::string Decoder::assembleInstructions(const std::vector<char> &bytestream) {
  // Build the instruction registry (could be cached if needed)
  auto registry = createInstructionRegistry();

  // add prefix to tell assembler we are using 8086
  std::stringstream ss;
  ss << "bits 16\n\n";

  uint32_t i = 0;
  while (i < bytestream.size()) {
    uint8_t opcode = static_cast<uint8_t>(bytestream[i]);
    bool found = false;

    // Try to match against registered patterns
    for (const auto &[pattern, maskAndHandler] : registry) {
      const auto &[mask, handler] = maskAndHandler;
      if ((opcode & mask) == pattern) {
        // Track position before decode to capture the instruction
        size_t posBefore = ss.str().length();
        i += handler->decode(ss, bytestream, i);
        size_t posAfter = ss.str().length();
        
        // Print the instruction that was just decoded
        printDecodedInstruction(ss, posBefore, posAfter);
        
        found = true;
        break;
      }
    }

    if (!found) {
      throw std::runtime_error(
          "Error decoding instruction: " + std::to_string(opcode) + " (0b" +
          std::bitset<8>(opcode).to_string() + ")");
    }
  }

  return ss.str();
}