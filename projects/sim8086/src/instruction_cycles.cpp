#include <sim8086/instruction_cycles.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

// Base cycle counts for each instruction mnemonic (register-register case)
// From Intel 8086 CPU reference manual
const std::unordered_map<std::string, uint32_t> InstructionCycles::BASE_CYCLES = {
    // Data movement
    {"MOV", 2},
    
    // Arithmetic operations
    {"ADD", 3},
    {"SUB", 3},
    {"CMP", 3},
    
    // Logical operations
    {"AND", 3},
    {"OR", 3},
    {"XOR", 3},
};

std::string InstructionCycles::extractMnemonic(const std::string &instruction) {
  // Extract the first word (mnemonic)
  std::istringstream iss(instruction);
  std::string mnemonic;
  iss >> mnemonic;

  // Convert to uppercase for consistency
  std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(),
                 [](unsigned char c) { return std::toupper(c); });

  return mnemonic;
}


bool InstructionCycles::hasMemoryAccess(const std::string &instruction) {
  // Memory access is indicated by square brackets
  return instruction.find('[') != std::string::npos;
}

// Calculate EA cycles based on addressing mode
static uint32_t calculateEACycles(const std::string &operand) {
  if (operand.find('[') == std::string::npos) {
    return 0; // Not a memory operand
  }
  
  size_t bracketStart = operand.find('[');
  size_t bracketEnd = operand.find(']');
  std::string content = operand.substr(bracketStart + 1, bracketEnd - bracketStart - 1);
  
  // Check for register combinations
  bool hasBP = content.find("BP") != std::string::npos;
  bool hasBX = content.find("BX") != std::string::npos;
  bool hasSI = content.find("SI") != std::string::npos;
  bool hasDI = content.find("DI") != std::string::npos;
  
  bool hasPlus = content.find('+') != std::string::npos;
  bool hasMinus = content.find('-') != std::string::npos;
  bool hasDisplacement = hasPlus || hasMinus;
  
  int registerCount = (hasBP ? 1 : 0) + (hasBX ? 1 : 0) + (hasSI ? 1 : 0) + (hasDI ? 1 : 0);
  
  // Two registers (base + index)
  if (registerCount == 2) {
    if (hasDisplacement) {
      // Displacement + base + index: 11 or 12 cycles
      if ((hasBP && hasDI) || (hasBX && hasSI)) {
        return 11; // BP+DI or BX+SI
      } else {
        return 12; // BP+SI or BX+DI
      }
    } else {
      // Base + index only: 7 or 8 cycles
      if ((hasBP && hasDI) || (hasBX && hasSI)) {
        return 7; // BP+DI or BX+SI
      } else {
        return 8; // BP+SI or BX+DI
      }
    }
  }
  
  // One register (base or index only)
  if (registerCount == 1) {
    if (hasDisplacement) {
      return 9; // Displacement + base/index
    } else {
      return 5; // Base or index only
    }
  }
  
  // No registers - displacement only or direct address
  // Direct addressing: [1000], [+1000]
  return 6; // Direct memory addressing
}

// Helper to check operand types
static bool isRegisterOperand(const std::string &operand) {
  return !operand.empty() && std::isalpha(operand[0]) && 
         operand.find('[') == std::string::npos;
}

static bool isImmediateOperand(const std::string &operand) {
  return !operand.empty() && operand.find('[') == std::string::npos &&
         (std::isdigit(operand[0]) || 
          (operand[0] == '-' && operand.size() > 1 && std::isdigit(operand[1])));
}

static bool isMemoryOperand(const std::string &operand) {
  return operand.find('[') != std::string::npos;
}

// MOV instruction cycle calculation
static CycleBreakdown calculateMovCycles(const std::string &destOp, const std::string &srcOp) {
  bool destIsAccumulator = (destOp == "AX" || destOp == "AL");
  bool srcIsAccumulator = (srcOp == "AX" || srcOp == "AL");
  bool hasMemorySource = isMemoryOperand(srcOp);
  bool hasMemoryDest = isMemoryOperand(destOp);
  bool isImmediate = isImmediateOperand(srcOp);
  bool isRegister = isRegisterOperand(srcOp);
  
  // MOV memory, accumulator = 10 cycles
  if (hasMemoryDest && srcIsAccumulator) {
    return {10, 0, 10};
  }
  
  // MOV accumulator, memory = 10 cycles
  if (destIsAccumulator && hasMemorySource) {
    return {10, 0, 10};
  }
  
  // MOV reg, reg = 2 cycles
  if (isRegister && !hasMemorySource && !hasMemoryDest) {
    return {2, 0, 2};
  }
  
  // MOV reg, immediate = 4 cycles
  if (isImmediate && !hasMemoryDest) {
    return {4, 0, 4};
  }
  
  // MOV [addr], immediate = 10 + EA
  if (hasMemoryDest && isImmediate) {
    uint32_t eaCycles = calculateEACycles(destOp);
    return {10, eaCycles, 10 + eaCycles};
  }
  
  // MOV reg, [addr] = 8 + EA
  if (hasMemorySource && !hasMemoryDest) {
    uint32_t eaCycles = calculateEACycles(srcOp);
    return {8, eaCycles, 8 + eaCycles};
  }
  
  // MOV [addr], reg = 9 + EA
  if (hasMemoryDest) {
    uint32_t eaCycles = calculateEACycles(destOp);
    return {9, eaCycles, 9 + eaCycles};
  }
  
  return {2, 0, 2}; // Default
}

// Arithmetic/Logical instructions (ADD, SUB, CMP, AND, OR, XOR)
static CycleBreakdown calculateArithmeticCycles(const std::string &destOp, const std::string &srcOp) {
  bool hasMemorySource = isMemoryOperand(srcOp);
  bool hasMemoryDest = isMemoryOperand(destOp);
  bool isImmediate = isImmediateOperand(srcOp);
  bool isRegister = isRegisterOperand(srcOp);
  
  // Immediate operands
  if (isImmediate) {
    if (hasMemoryDest) {
      uint32_t eaCycles = calculateEACycles(destOp);
      return {17, eaCycles, 17 + eaCycles}; // memory, immediate = 17 + EA
    } else {
      return {4, 0, 4}; // register, immediate = 4 cycles
    }
  }
  
  // Memory operands
  if (hasMemorySource && !hasMemoryDest) {
    uint32_t eaCycles = calculateEACycles(srcOp);
    return {9, eaCycles, 9 + eaCycles}; // register, memory = 9 + EA
  }
  if (hasMemoryDest) {
    uint32_t eaCycles = calculateEACycles(destOp);
    return {16, eaCycles, 16 + eaCycles}; // memory, register = 16 + EA
  }
  
  // Register to register
  if (isRegister && !hasMemorySource && !hasMemoryDest) {
    return {3, 0, 3}; // register, register = 3 cycles
  }
  
  return {3, 0, 3}; // Default
}

// Dispatch to mnemonic-specific calculator
static CycleBreakdown calculateCyclesInternal(const Instruction &instr) {
  // Jump instructions (don't modify registers, just timing)
  if (instr.mnemonic[0] == 'J' || instr.mnemonic == "LOOP" || 
      instr.mnemonic == "LOOPZ" || instr.mnemonic == "LOOPNZ") {
    // Jumps have fixed timing (worst case - not taken)
    uint32_t cycles = 16;
    if (instr.mnemonic == "JMP") cycles = 15;
    if (instr.mnemonic == "LOOP" || instr.mnemonic == "LOOPZ" || instr.mnemonic == "LOOPNZ") cycles = 17;
    return {cycles, 0, cycles};
  }
  
  // Dispatch to mnemonic-specific handlers
  if (instr.mnemonic == "MOV") {
    return calculateMovCycles(instr.destOp, instr.srcOp);
  }
  
  if (instr.mnemonic == "ADD" || instr.mnemonic == "SUB" || instr.mnemonic == "CMP" ||
      instr.mnemonic == "AND" || instr.mnemonic == "OR" || instr.mnemonic == "XOR") {
    return calculateArithmeticCycles(instr.destOp, instr.srcOp);
  }
  
  // Unsupported instruction
  throw std::runtime_error("Unsupported instruction: " + instr.mnemonic);
}

CycleBreakdown InstructionCycles::getCycleBreakdown(const Instruction &instruction) {
  return calculateCyclesInternal(instruction);
}

