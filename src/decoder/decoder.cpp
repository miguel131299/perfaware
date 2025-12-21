#include <bitset>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <sim8086/decoder.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

#include "instructions/add_immediate_accumulator_handler.hpp"
#include "instructions/add_regmem_handler.hpp"
#include "instructions/cmp_immediate_accumulator_handler.hpp"
#include "instructions/cmp_regmem_handler.hpp"
#include "instructions/immediate_regmem_group_handler.hpp"
#include "instructions/instruction_handler.hpp"
#include "instructions/mov_accumulator_mem_handler.hpp"
#include "instructions/mov_immediate_handler.hpp"
#include "instructions/mov_immediate_regmem_handler.hpp"
#include "instructions/mov_mem_accumulator_handler.hpp"
#include "instructions/mov_regmem_handler.hpp"
#include "instructions/sub_immediate_accumulator_handler.hpp"
#include "instructions/sub_regmem_handler.hpp"

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

  auto movMemAccumulatorHandler = std::make_unique<MOVMemAccumulatorHandler>();
  registry[0b10100000] = {0b11111110, std::move(movMemAccumulatorHandler)};

  auto movAccumulatorMemHandler = std::make_unique<MOVAccumulatorMemHandler>();
  registry[0b10100010] = {0b11111110, std::move(movAccumulatorMemHandler)};

  // ADD instructions (each pattern gets its own handler)
  auto addRegMemHandler = std::make_unique<ADDRegMemHandler>();
  registry[0b00000000] = {0b11111100, std::move(addRegMemHandler)};

  auto addImmediateAccumulatorHandler =
      std::make_unique<ADDImmediateAccumulatorHandler>();
  registry[0b00000100] = {0b11111110,
                          std::move(addImmediateAccumulatorHandler)};

  // SUB instructions (each pattern gets its own handler)
  auto subRegMemHandler = std::make_unique<SUBRegMemHandler>();
  registry[0b00101000] = {0b11111100, std::move(subRegMemHandler)};

  auto subImmediateAccumulatorHandler =
      std::make_unique<SUBImmediateAccumulatorHandler>();
  registry[0b00101100] = {0b11111110,
                          std::move(subImmediateAccumulatorHandler)};

  // CMP instructions (each pattern gets its own handler)
  auto cmpRegMemHandler = std::make_unique<CMPRegMemHandler>();
  registry[0b00111000] = {0b11111100, std::move(cmpRegMemHandler)};

  auto cmpImmediateAccumulatorHandler =
      std::make_unique<CMPImmediateAccumulatorHandler>();
  registry[0b00111100] = {0b11111110,
                          std::move(cmpImmediateAccumulatorHandler)};

  // Group handler for 0x80-0x83 immediate to reg/mem (ADD, SUB, CMP)
  auto immediateRegMemGroupHandler =
      std::make_unique<ImmediateRegMemGroupHandler>();
  registry[0b10000000] = {0b11111100, std::move(immediateRegMemGroupHandler)};

  // TODO: Add more instruction types here
  // auto xorHandler = std::make_unique<XORHandler>();
  // registry[0b00110000] = {0b11111100, std::move(xorHandler)};

  return registry;
}

//-----------------------------------------------------------------------------
// Print the instruction that was just decoded to stderr for debugging
static void printDecodedInstruction(const std::stringstream &ss,
                                    size_t posBefore, size_t posAfter) {
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