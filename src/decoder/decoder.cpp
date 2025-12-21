#include <bitset>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
#include <set>
#include <sim8086/decoder.hpp>
#include <sstream>
#include <stdexcept>
#include <string>

#include "instructions/arithmetic/arithmetic_immediate_accumulator_handler.hpp"
#include "instructions/arithmetic/immediate_regmem_group_handler.hpp"
#include "instructions/generic/generic_regmem_handler.hpp"
#include "instructions/jumps/generic_jump_handler.hpp"
#include "instructions/mov/mov_accumulator_mem_handler.hpp"
#include "instructions/mov/mov_immediate_handler.hpp"
#include "instructions/mov/mov_immediate_regmem_handler.hpp"
#include "instructions/mov/mov_mem_accumulator_handler.hpp"
#include "instructions/utilities/instruction_handler.hpp"
#include "instructions/utilities/instruction_output_utilities.hpp"

//-----------------------------------------------------------------------------
// Create instruction handler registry
// Maps instruction patterns to their handlers
static std::map<uint8_t,
                std::pair<uint8_t, std::unique_ptr<InstructionHandler>>>
createInstructionRegistry() {
  std::map<uint8_t, std::pair<uint8_t, std::unique_ptr<InstructionHandler>>>
      registry;

  // TODO: Deduplicate MOV and arithmetic handlers

  // MOV instructions (reg/mem with register uses generic handler)
  auto movRegMemHandler = std::make_unique<GenericRegMemHandler>(
      "MOV (reg/mem)",
      [](std::stringstream &ss, const std::string &dest,
         const std::string &src) { outputMOVInstruction(ss, dest, src); });
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

  // ADD instructions (generic handler for reg/mem)
  auto addRegMemHandler = std::make_unique<GenericRegMemHandler>(
      "ADD (reg/mem)",
      [](std::stringstream &ss, const std::string &dest,
         const std::string &src) { outputInstruction(ss, "add", dest, src); });
  registry[0b00000000] = {0b11111100, std::move(addRegMemHandler)};

  auto addImmediateAccumulatorHandler =
      std::make_unique<ArithmeticImmediateAccumulatorHandler>(
          "ADD (imm to accumulator)",
          [](std::stringstream &ss, const std::string &dest,
             const std::string &src) {
            outputInstruction(ss, "add", dest, src);
          });
  registry[0b00000100] = {0b11111110,
                          std::move(addImmediateAccumulatorHandler)};

  // SUB instructions (generic handler for reg/mem)
  auto subRegMemHandler = std::make_unique<GenericRegMemHandler>(
      "SUB (reg/mem)",
      [](std::stringstream &ss, const std::string &dest,
         const std::string &src) { outputInstruction(ss, "sub", dest, src); });
  registry[0b00101000] = {0b11111100, std::move(subRegMemHandler)};

  auto subImmediateAccumulatorHandler =
      std::make_unique<ArithmeticImmediateAccumulatorHandler>(
          "SUB (imm to accumulator)",
          [](std::stringstream &ss, const std::string &dest,
             const std::string &src) {
            outputInstruction(ss, "sub", dest, src);
          });
  registry[0b00101100] = {0b11111110,
                          std::move(subImmediateAccumulatorHandler)};

  // CMP instructions (generic handler for reg/mem)
  auto cmpRegMemHandler = std::make_unique<GenericRegMemHandler>(
      "CMP (reg/mem)",
      [](std::stringstream &ss, const std::string &dest,
         const std::string &src) { outputInstruction(ss, "cmp", dest, src); });
  registry[0b00111000] = {0b11111100, std::move(cmpRegMemHandler)};

  auto cmpImmediateAccumulatorHandler =
      std::make_unique<ArithmeticImmediateAccumulatorHandler>(
          "CMP (imm to accumulator)",
          [](std::stringstream &ss, const std::string &dest,
             const std::string &src) {
            outputInstruction(ss, "cmp", dest, src);
          });
  registry[0b00111100] = {0b11111110,
                          std::move(cmpImmediateAccumulatorHandler)};

  // Group handler for 0x80-0x83 immediate to reg/mem (ADD, SUB, CMP)
  auto immediateRegMemGroupHandler =
      std::make_unique<ImmediateRegMemGroupHandler>();
  registry[0b10000000] = {0b11111100, std::move(immediateRegMemGroupHandler)};

  // Jump instructions (all use generic jump handler with 8-bit displacement)
  registry[0x70] = {0xFF, std::make_unique<GenericJumpHandler>("jo")};
  registry[0x71] = {0xFF, std::make_unique<GenericJumpHandler>("jno")};
  registry[0x72] = {0xFF, std::make_unique<GenericJumpHandler>("jb")};
  registry[0x73] = {0xFF, std::make_unique<GenericJumpHandler>("jnb")};
  registry[0x74] = {0xFF, std::make_unique<GenericJumpHandler>("je")};
  registry[0x75] = {0xFF, std::make_unique<GenericJumpHandler>("jnz")};
  registry[0x76] = {0xFF, std::make_unique<GenericJumpHandler>("jbe")};
  registry[0x77] = {0xFF, std::make_unique<GenericJumpHandler>("ja")};
  registry[0x78] = {0xFF, std::make_unique<GenericJumpHandler>("js")};
  registry[0x79] = {0xFF, std::make_unique<GenericJumpHandler>("jns")};
  registry[0x7A] = {0xFF, std::make_unique<GenericJumpHandler>("jp")};
  registry[0x7B] = {0xFF, std::make_unique<GenericJumpHandler>("jnp")};
  registry[0x7C] = {0xFF, std::make_unique<GenericJumpHandler>("jl")};
  registry[0x7D] = {0xFF, std::make_unique<GenericJumpHandler>("jnl")};
  registry[0x7E] = {0xFF, std::make_unique<GenericJumpHandler>("jle")};
  registry[0x7F] = {0xFF, std::make_unique<GenericJumpHandler>("jg")};

  // Loop instructions (8-bit displacement)
  registry[0xE0] = {0xFF, std::make_unique<GenericJumpHandler>("loopnz")};
  registry[0xE1] = {0xFF, std::make_unique<GenericJumpHandler>("loopz")};
  registry[0xE2] = {0xFF, std::make_unique<GenericJumpHandler>("loop")};
  registry[0xE3] = {0xFF, std::make_unique<GenericJumpHandler>("jcxz")};

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
// Helper structure to track instruction offsets and their types
struct InstructionInfo {
  uint32_t byteOffset;
  uint32_t bytesLength;
  bool isJump;
  int32_t jumpTarget;        // Only valid if isJump is true
  std::string decodedOutput; // Cached output from decoding
};

// First pass: Decode instructions and collect jump target addresses
static std::vector<InstructionInfo> collectInstructions(
    const std::vector<char> &bytestream,
    const std::map<uint8_t,
                   std::pair<uint8_t, std::unique_ptr<InstructionHandler>>>
        &registry) {
  std::vector<InstructionInfo> instructions;
  instructions.reserve(bytestream.size() /
                       2); // Most instructions are 1-2 bytes

  uint32_t i = 0;
  while (i < bytestream.size()) {
    uint8_t opcode = static_cast<uint8_t>(bytestream[i]);
    bool found = false;

    // Try to match against registered patterns
    // TODO: Do efficient lookup table instead of iterating through all possible
    // patterns. (Sort by length of mask).
    for (const auto &[pattern, maskAndHandler] : registry) {
      const auto &[mask, handler] = maskAndHandler;
      if ((opcode & mask) == pattern) {
        uint32_t byteOffsetBefore = i;

        // Decode instruction and capture output
        std::stringstream decodedSs;
        i += handler->decode(decodedSs, bytestream, i);
        uint32_t instructionLength = i - byteOffsetBefore;

        // Check if this is a jump instruction (opcodes 0x70-0x7F, 0xE0-0xE3)
        bool isJump = (opcode >= 0x70 && opcode <= 0x7F) ||
                      (opcode >= 0xE0 && opcode <= 0xE3);

        InstructionInfo info{byteOffsetBefore, instructionLength, isJump, 0,
                             decodedSs.str()};

        // If jump, calculate target
        if (isJump) {
          int8_t displacement =
              static_cast<int8_t>(bytestream[byteOffsetBefore + 1]);
          int32_t target =
              static_cast<int32_t>(byteOffsetBefore + 2 + displacement);
          info.jumpTarget = target;
        }

        instructions.push_back(info);
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

  return instructions;
}

//-----------------------------------------------------------------------------
std::string Decoder::assembleInstructions(const std::vector<char> &bytestream) {
  // Cache the registry - build it once per program
  static const auto registry = createInstructionRegistry();

  // First pass: Collect all instructions and jump targets
  auto instructions = collectInstructions(bytestream, registry);

  // Build a set of all jump targets
  std::set<uint32_t> jumpTargets;
  for (const auto &instr : instructions) {
    if (instr.isJump) {
      jumpTargets.insert(instr.jumpTarget);
    }
  }

  // Assign label names to jump targets
  std::map<uint32_t, std::string> targetToLabel;
  int labelCounter = 0;
  for (uint32_t target : jumpTargets) {
    std::string labelName;
    if (labelCounter < 2) {
      labelName = "test_label" + std::to_string(labelCounter);
    } else {
      labelName = "label";
    }
    targetToLabel[target] = labelName;
    labelCounter++;
  }

  // Second pass: Generate assembly with labels
  std::stringstream ss;
  ss << "bits 16\n\n";

  for (const auto &instr : instructions) {
    // Output label if this offset is a jump target
    if (jumpTargets.count(instr.byteOffset)) {
      ss << targetToLabel[instr.byteOffset] << ":\n";
    }

    // Decode and output the instruction
    uint8_t opcode = static_cast<uint8_t>(bytestream[instr.byteOffset]);

    // Find the handler for this instruction
    for (const auto &[pattern, maskAndHandler] : registry) {
      const auto &[mask, handler] = maskAndHandler;
      if ((opcode & mask) == pattern) {
        size_t posBefore = ss.str().length();

        // For jump instructions, we need special handling to use labels instead
        // of offsets
        bool isJump = (opcode >= 0x70 && opcode <= 0x7F) ||
                      (opcode >= 0xE0 && opcode <= 0xE3);

        if (isJump) {
          // Handle jump manually with label - avoid expensive decode + string
          // parsing
          const char *mnemonic = nullptr;
          if (opcode >= 0x70 && opcode <= 0x7F) {
            static const char *jump_mn[] = {
                "jo", "jno", "jb", "jnb", "je", "jnz", "jbe", "ja",
                "js", "jns", "jp", "jnp", "jl", "jnl", "jle", "jg"};
            mnemonic = jump_mn[opcode - 0x70];
          } else if (opcode >= 0xE0 && opcode <= 0xE3) {
            static const char *loop_mn[] = {"loopnz", "loopz", "loop", "jcxz"};
            mnemonic = loop_mn[opcode - 0xE0];
          }

          int8_t displacement =
              static_cast<int8_t>(bytestream[instr.byteOffset + 1]);
          int32_t target =
              static_cast<int32_t>(instr.byteOffset + 2 + displacement);

          // Output with label (mnemonic should not be null)
          if (mnemonic) {
            ss << mnemonic << " " << targetToLabel[target] << "\n";
          }
        } else {
          // Regular instruction - use cached output from pass 1
          ss << instr.decodedOutput;
        }

        size_t posAfter = ss.str().length();
        printDecodedInstruction(ss, posBefore, posAfter);
        break;
      }
    }
  }

  return ss.str();
}