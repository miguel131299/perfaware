#pragma once

#include <cstdint>
#include <functional>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

class Simulator {
public:
  // Initialize simulator with bytecode to execute
  explicit Simulator(const std::vector<char> &bytestream, bool trackIPRegister = false);

  // Execute all instructions in the bytecode
  void run();

  // Execute a single instruction and advance IP
  void step();

  // Get current state for inspection
  std::string dumpState() const;

  // Get execution trace output
  std::string getTrace() const;

private:
  // Instruction representation
  struct Instruction {
    std::string mnemonic; // "mov", "add", "jne", etc.
    std::string destOp;   // destination operand string
    std::string srcOp;    // source operand string
  };

  // Decoded instruction representation
  struct DecodedInstruction {
    uint32_t byteOffset;
    std::string decoded;
    Instruction parsed;
  };

  // CPU State
  struct Registers {
    uint16_t ax = 0, bx = 0, cx = 0, dx = 0;
    uint16_t sp = 0, bp = 0, si = 0, di = 0;
    uint32_t ip = 0;  // Instruction pointer (byte offset into bytestream)

    struct Flags {
      bool zero = false;
      bool sign = false;
      bool overflow = false;
      bool carry = false;
      bool parity = false;
      bool auxiliary = false;  // Auxiliary Carry flag (AF)
    } flags;
  } registers;

  // Memory (64KB for 8086)
  std::vector<uint8_t> memory;

  // Bytecode to execute
  const std::vector<char> &bytestream;

  // Whether to track IP register changes in trace output
  bool trackIPRegister = false;

  // Trace output stream for execution steps
  std::ostringstream traceOutput;

  // Decode and execute a single instruction at the current instruction pointer
  // Returns false if at end of bytecode
  bool decodeAndExecuteStep();

  // Parse a decoded instruction string into structured format
  // E.g., "mov AX, BX" -> Instruction{"mov", "AX", "BX"}
  Instruction parseInstruction(const std::string &decoded);

  // Execute an instruction and return the next instruction pointer
  uint32_t executeInstruction(const Instruction &instr, uint32_t currentIP,
                              uint32_t nextIP);

  // Instruction executors
  void executeMov(const std::string &dest, const std::string &src);
  void executeAdd(const std::string &dest, const std::string &src);
  void executeSub(const std::string &dest, const std::string &src);
  void executeCmp(const std::string &dest, const std::string &src);
  uint32_t executeJump(const std::string &mnemonic, uint32_t currentIP,
                       uint32_t nextIP);

  // Helper: Perform subtraction with flag updates (shared by SUB and CMP)
  void performSubtraction(const std::string &dest, const std::string &src,
                          bool storeResult);

  // Helper: Resolve operand value
  // "AX" -> 0x1234, "[BX + SI]" -> memory[bx+si], "5" -> 5, etc.
  uint16_t resolveOperand(const std::string &operand);

  // Helper: Set operand value
  // "AX" <- 0x1234, "[BX]" <- 0x5678, etc.
  void setOperand(const std::string &operand, uint16_t value);

  // Helper: Update flags after arithmetic operations
  void setFlags(uint16_t result, bool carry = false, bool overflow = false);

  // Helper: Flag update functions for individual flags
  void handleZeroFlag(uint16_t val);
  void handleSignFlag(uint16_t val);
  void handleCarryFlag(bool carry);
  void handleOverflowFlag(bool overflow);
  void handleParityFlag(uint16_t val);
  void handleAuxiliaryFlag(uint16_t oldVal, uint16_t newVal);

  // Helper: Get flag state as string (e.g., "SPZ" for Sign, Parity, Zero)
  std::string getFlagString() const;

  // Helper: Check if a jump condition is met
  bool shouldJump(const std::string &mnemonic) const;

  // Helper: Read signed displacement byte from bytecode at given offset
  int8_t readDisplacement(uint32_t offset);

  // Helper: Calculate jump target from current IP and displacement
  uint32_t calculateJumpTarget(uint32_t currentIP, int8_t displacement);

  // Helper: Get register value by name
  // "AX" -> 0x1234, "AL" -> 0x34 (lower byte), "AH" -> 0x12 (upper byte)
  uint16_t getRegisterValue(const std::string &regName) const;

  // Helper: Set register value by name
  // "AX" <- 0x1234, "AL" <- 0x34 (preserves upper byte), "AH" <- 0x12
  void setRegisterValue(const std::string &regName, uint16_t value);

  // Helper: Parse immediate value from string
  // "5" -> 5, "-30" -> 0xFFE2 (two's complement), "0x1234" -> 0x1234
  uint16_t parseImmediate(const std::string &immediate) const;

  // Helper: Get the name of a register from string
  // "AX" -> ax field address, "BL" -> lower byte of ax, etc.
  uint16_t &getRegister(const std::string &regName);

  // Helper: Operand type checking
  bool isRegister(const std::string &operand) const;
  bool isMemory(const std::string &operand) const;
  bool isImmediate(const std::string &operand) const;

  // Helper: Parse memory address from operand string
  // "[5]" -> 5, "[BX]" -> registers.bx, "[BX + 4]" -> registers.bx + 4, etc.
  uint16_t parseMemoryAddress(const std::string &operand) const;

  // Per-instance register access maps (capture this safely, not static)
  std::unordered_map<std::string, std::function<uint16_t()>> getRegisterMap;
  std::unordered_map<std::string, std::function<void(uint16_t)>> setRegisterMap;
};
