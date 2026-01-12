#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

// Parsed instruction structure
struct Instruction {
  std::string mnemonic;
  std::string destOp;
  std::string srcOp;
};

// Structure to hold cycle breakdown information
struct CycleBreakdown {
  uint32_t baseCycles;
  uint32_t eaCycles;
  uint32_t totalCycles;  // baseCycles + eaCycles
};

class InstructionCycles {
public:
  /**
   * Get cycle breakdown (base cycles + EA cycles).
   * This is the primary function - all cycle information in one call.
   */
  static CycleBreakdown getCycleBreakdown(const Instruction &instruction);

private:
  // Base cycles for each mnemonic (assuming register-to-register)
  static const std::unordered_map<std::string, uint32_t> BASE_CYCLES;

  // Extract mnemonic from instruction string
  static std::string extractMnemonic(const std::string &instruction);

  // Check if instruction has memory access
  static bool hasMemoryAccess(const std::string &instruction);
};
