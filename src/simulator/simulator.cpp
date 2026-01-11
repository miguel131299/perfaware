#include "sim8086/simulator.hpp"
#include "sim8086/decoder.hpp"

#include <cctype>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

Simulator::Simulator(const std::vector<char> &bytestream, bool trackIPRegister)
    : memory(65536, 0), bytestream(bytestream),
      trackIPRegister(trackIPRegister) {
  // Initialize register getter map - each instance has its own capturing this
  getRegisterMap = {
      {"AX", [this]() { return registers.ax; }},
      {"BX", [this]() { return registers.bx; }},
      {"CX", [this]() { return registers.cx; }},
      {"DX", [this]() { return registers.dx; }},
      {"SP", [this]() { return registers.sp; }},
      {"BP", [this]() { return registers.bp; }},
      {"SI", [this]() { return registers.si; }},
      {"DI", [this]() { return registers.di; }},
      {"AH", [this]() { return (registers.ax >> 8) & 0xFF; }},
      {"BH", [this]() { return (registers.bx >> 8) & 0xFF; }},
      {"CH", [this]() { return (registers.cx >> 8) & 0xFF; }},
      {"DH", [this]() { return (registers.dx >> 8) & 0xFF; }},
      {"AL", [this]() { return registers.ax & 0xFF; }},
      {"BL", [this]() { return registers.bx & 0xFF; }},
      {"CL", [this]() { return registers.cx & 0xFF; }},
      {"DL", [this]() { return registers.dx & 0xFF; }},
  };

  // Initialize register setter map - each instance has its own capturing this
  setRegisterMap = {
      {"AX", [this](uint16_t v) { registers.ax = v; }},
      {"BX", [this](uint16_t v) { registers.bx = v; }},
      {"CX", [this](uint16_t v) { registers.cx = v; }},
      {"DX", [this](uint16_t v) { registers.dx = v; }},
      {"SP", [this](uint16_t v) { registers.sp = v; }},
      {"BP", [this](uint16_t v) { registers.bp = v; }},
      {"SI", [this](uint16_t v) { registers.si = v; }},
      {"DI", [this](uint16_t v) { registers.di = v; }},
      {"AH",
       [this](uint16_t v) {
         registers.ax = (registers.ax & 0xFF) | ((v & 0xFF) << 8);
       }},
      {"BH",
       [this](uint16_t v) {
         registers.bx = (registers.bx & 0xFF) | ((v & 0xFF) << 8);
       }},
      {"CH",
       [this](uint16_t v) {
         registers.cx = (registers.cx & 0xFF) | ((v & 0xFF) << 8);
       }},
      {"DH",
       [this](uint16_t v) {
         registers.dx = (registers.dx & 0xFF) | ((v & 0xFF) << 8);
       }},
      {"AL",
       [this](uint16_t v) {
         registers.ax = (registers.ax & 0xFF00) | (v & 0xFF);
       }},
      {"BL",
       [this](uint16_t v) {
         registers.bx = (registers.bx & 0xFF00) | (v & 0xFF);
       }},
      {"CL",
       [this](uint16_t v) {
         registers.cx = (registers.cx & 0xFF00) | (v & 0xFF);
       }},
      {"DL",
       [this](uint16_t v) {
         registers.dx = (registers.dx & 0xFF00) | (v & 0xFF);
       }},
  };
}

void Simulator::run() {
  // Execute instructions until we reach end of bytecode
  while (decodeAndExecuteStep()) {
  }
}

void Simulator::step() { decodeAndExecuteStep(); }

bool Simulator::decodeAndExecuteStep() {
  if (registers.ip >= bytestream.size()) {
    return false; // End of program
  }

  // Capture old IP before execution
  uint32_t oldIP = registers.ip;

  // Decode one instruction at current IP
  auto [decodedStr, nextIP] =
      Decoder::decodeOneInstruction(bytestream, registers.ip);

  if (decodedStr.empty()) {
    return false; // End of program
  }

  // Remove trailing newline from decoded string if present
  if (!decodedStr.empty() && decodedStr.back() == '\n') {
    decodedStr.pop_back();
  }

  // Parse decoded instruction
  Instruction parsed = parseInstruction(decodedStr);

  // Capture old register value before execution (for MOV/ADD/SUB operations)
  // CMP doesn't modify registers, so skip tracking for it
  uint16_t oldValue = 0;
  bool trackRegister = false;
  std::string regName;

  if (isRegister(parsed.destOp) && parsed.mnemonic != "CMP") {
    regName = parsed.destOp;
    oldValue = getRegisterValue(regName);
    trackRegister = true;
  }

  // Capture old flag state before execution
  std::string oldFlags = getFlagString();

  // Execute instruction and get next IP
  uint32_t actualNextIP = executeInstruction(parsed, registers.ip, nextIP);
  registers.ip = actualNextIP;

  // Get new flag state after execution
  std::string newFlags = getFlagString();

  // Output trace
  traceOutput << decodedStr;

  // Track whether we've started the comment section
  bool hasComment = false;

  if (trackRegister) {
    uint16_t newValue = getRegisterValue(regName);
    // Only track if value actually changed
    if (oldValue != newValue) {
      traceOutput << " ; " << regName << ":0x" << std::hex << oldValue << "->0x"
                  << newValue << std::dec;
      hasComment = true;
    }
  }

  // Output IP changes if tracking is enabled
  if (trackIPRegister) {
    if (!hasComment) {
      traceOutput << " ;";
      hasComment = true;
    }
    traceOutput << " ip:0x" << std::hex << oldIP << "->0x" << actualNextIP
                << std::dec;
  }

  // Output flag changes if any
  if (oldFlags != newFlags) {
    if (!hasComment) {
      traceOutput << " ;";
    }
    traceOutput << " flags:" << oldFlags << "->" << newFlags;
  }

  traceOutput << "\n";

  return true;
}

std::string Simulator::dumpState() const {
  std::ostringstream oss;
  oss << "Final registers:\n";

  // Only output non-zero registers
  if (registers.ax != 0) {
    oss << "      ax: 0x" << std::hex << std::setfill('0') << std::setw(4)
        << registers.ax << " (" << std::dec << registers.ax << ")\n";
  }
  if (registers.bx != 0) {
    oss << "      bx: 0x" << std::hex << std::setfill('0') << std::setw(4)
        << registers.bx << " (" << std::dec << registers.bx << ")\n";
  }
  if (registers.cx != 0) {
    oss << "      cx: 0x" << std::hex << std::setfill('0') << std::setw(4)
        << registers.cx << " (" << std::dec << registers.cx << ")\n";
  }
  if (registers.dx != 0) {
    oss << "      dx: 0x" << std::hex << std::setfill('0') << std::setw(4)
        << registers.dx << " (" << std::dec << registers.dx << ")\n";
  }
  if (registers.sp != 0) {
    oss << "      sp: 0x" << std::hex << std::setfill('0') << std::setw(4)
        << registers.sp << " (" << std::dec << registers.sp << ")\n";
  }
  if (registers.bp != 0) {
    oss << "      bp: 0x" << std::hex << std::setfill('0') << std::setw(4)
        << registers.bp << " (" << std::dec << registers.bp << ")\n";
  }
  if (registers.si != 0) {
    oss << "      si: 0x" << std::hex << std::setfill('0') << std::setw(4)
        << registers.si << " (" << std::dec << registers.si << ")\n";
  }
  if (registers.di != 0) {
    oss << "      di: 0x" << std::hex << std::setfill('0') << std::setw(4)
        << registers.di << " (" << std::dec << registers.di << ")\n";
  }

  // Output IP register if tracking is enabled
  if (trackIPRegister) {
    oss << "      ip: 0x" << std::hex << std::setfill('0') << std::setw(4)
        << registers.ip << " (" << std::dec << registers.ip << ")\n";
  }

  // Output final flags if any are set
  std::string flags = getFlagString();
  if (!flags.empty()) {
    oss << "   flags: " << flags << "\n";
  }

  return oss.str();
}

std::string Simulator::getTrace() const { return traceOutput.str(); }

std::string Simulator::getFlagString() const {
  // Build flag string with flags in order: C, P, A, Z, S, O
  // For this simulator we track: C (carry), P (parity), A (auxiliary), Z
  // (zero), S (sign), O (overflow)
  std::string flags;
  if (registers.flags.carry)
    flags += "C";
  if (registers.flags.parity)
    flags += "P";
  if (registers.flags.auxiliary)
    flags += "A";
  if (registers.flags.zero)
    flags += "Z";
  if (registers.flags.sign)
    flags += "S";
  if (registers.flags.overflow)
    flags += "O";
  return flags;
}

Simulator::Instruction Simulator::parseInstruction(const std::string &decoded) {
  // Lambda to trim whitespace (returns trimmed copy)
  auto trim = [](std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
  };

  // Trim input and find mnemonic
  std::string trimmed = trim(decoded);
  size_t spacePos = trimmed.find(' ');

  if (spacePos == std::string::npos) {
    // Just a mnemonic, no operands
    return Instruction{trimmed, "", ""};
  }

  std::string mnemonic = trimmed.substr(0, spacePos);
  std::string operands = trimmed.substr(spacePos + 1);

  // Split operands by comma
  size_t commaPos = operands.find(',');
  std::string destOp, srcOp;

  if (commaPos == std::string::npos) {
    // No comma - single operand (like jump instructions)
    destOp = trim(operands);
    srcOp = "";
  } else {
    destOp = trim(operands.substr(0, commaPos));
    srcOp = trim(operands.substr(commaPos + 1));
  }

  return Instruction{mnemonic, destOp, srcOp};
}

uint32_t Simulator::executeInstruction(const Instruction &instr,
                                       uint32_t currentIP, uint32_t nextIP) {
  // TODO: Dispatch to appropriate executor based on mnemonic
  if (instr.mnemonic == "MOV") {
    executeMov(instr.destOp, instr.srcOp);
  } else if (instr.mnemonic == "ADD") {
    executeAdd(instr.destOp, instr.srcOp);
  } else if (instr.mnemonic == "SUB") {
    executeSub(instr.destOp, instr.srcOp);
  } else if (instr.mnemonic == "CMP") {
    executeCmp(instr.destOp, instr.srcOp);
  } else if (instr.mnemonic[0] == 'J') {
    // Jumps return potentially different next IP
    return executeJump(instr.mnemonic, currentIP, nextIP);
  }
  // Non-jump instructions advance normally
  return nextIP;
}

void Simulator::handleZeroFlag(uint16_t val) {
  registers.flags.zero = (val == 0);
}

void Simulator::handleSignFlag(uint16_t val) {
  registers.flags.sign = ((val & 0x8000) != 0);
}

void Simulator::handleCarryFlag(bool carry) { registers.flags.carry = carry; }

void Simulator::handleOverflowFlag(bool overflow) {
  registers.flags.overflow = overflow;
}

void Simulator::executeMov(const std::string &dest, const std::string &src) {
  // 1. Resolve source operand to a value
  uint16_t srcVal = resolveOperand(src);
  // 2. Set destination operand to that value
  setOperand(dest, srcVal);
}

void Simulator::handleParityFlag(uint16_t val) {
  // Parity is based on lower 8 bits - count number of 1 bits
  uint8_t lowByte = val & 0xFF;
  int count = 0;
  while (lowByte) {
    count += lowByte & 1;
    lowByte >>= 1;
  }
  // Parity flag set if even number of 1 bits
  registers.flags.parity = (count % 2 == 0);
}

void Simulator::handleAuxiliaryFlag(uint16_t oldVal, uint16_t newVal) {
  // Auxiliary Carry flag (AF) is set if there was a carry from bit 3 to bit 4
  // in the lower nibble (4 bits)
  uint16_t carry = (oldVal & 0xF) + (newVal & 0xF);
  registers.flags.auxiliary = (carry > 0xF);
}

void Simulator::executeAdd(const std::string &dest, const std::string &src) {
  uint16_t srcVal = resolveOperand(src);
  uint16_t destVal = resolveOperand(dest);

  // Calculate result and detect carry/overflow
  uint32_t result32 =
      static_cast<uint32_t>(destVal) + static_cast<uint32_t>(srcVal);
  uint16_t result = static_cast<uint16_t>(result32);

  // Carry: result overflowed 16 bits
  bool carry = (result32 > 0xFFFF);

  // Overflow: sign of operands same, but result sign different
  bool destSign = (destVal & 0x8000) != 0;
  bool srcSign = (srcVal & 0x8000) != 0;
  bool resultSign = (result & 0x8000) != 0;
  bool overflow = (destSign == srcSign) && (destSign != resultSign);

  // Auxiliary carry: carry from bit 3 to bit 4
  uint16_t auxCarry = ((destVal & 0xF) + (srcVal & 0xF)) & 0x10;

  setOperand(dest, result);
  setFlags(result, carry, overflow);
  registers.flags.auxiliary = (auxCarry != 0);
}

void Simulator::executeSub(const std::string &dest, const std::string &src) {
  performSubtraction(dest, src, true);
}

void Simulator::executeCmp(const std::string &dest, const std::string &src) {
  performSubtraction(dest, src, false);
}

uint32_t Simulator::executeJump(const std::string &mnemonic, uint32_t currentIP,
                                uint32_t nextIP) {
  if (shouldJump(mnemonic)) {
    int8_t displacement = readDisplacement(currentIP);
    return calculateJumpTarget(currentIP, displacement);
  }
  return nextIP; // No jump condition met, return normal next instruction
}

void Simulator::performSubtraction(const std::string &dest,
                                   const std::string &src, bool storeResult) {
  uint16_t srcVal = resolveOperand(src);
  uint16_t destVal = resolveOperand(dest);

  // Calculate result and detect borrow (carry) and overflow
  int32_t result32 =
      static_cast<int32_t>(destVal) - static_cast<int32_t>(srcVal);
  uint16_t result = static_cast<uint16_t>(result32);

  // Carry/Borrow: result underflowed (destVal < srcVal in unsigned)
  bool carry = (destVal < srcVal);

  // Overflow: signs different in operands, result sign different from dest
  bool destSign = (destVal & 0x8000) != 0;
  bool srcSign = (srcVal & 0x8000) != 0;
  bool resultSign = (result & 0x8000) != 0;
  bool overflow = (destSign != srcSign) && (destSign != resultSign);

  // Auxiliary carry: borrow from bit 4 in lower nibble
  uint16_t auxBorrow = (destVal & 0xF) < (srcVal & 0xF) ? 1 : 0;

  // Only store result if this is SUB, not CMP
  if (storeResult) {
    setOperand(dest, result);
  }

  // Always update flags
  setFlags(result, carry, overflow);
  registers.flags.auxiliary = (auxBorrow != 0);
}

uint16_t Simulator::resolveOperand(const std::string &operand) {
  if (isRegister(operand)) {
    return getRegisterValue(operand);
  } else if (isMemory(operand)) {
    uint16_t addr = parseMemoryAddress(operand);
    // Read 16-bit word from memory (little-endian)
    uint16_t value =
        static_cast<uint16_t>(memory[addr]) |
        (static_cast<uint16_t>(memory[(addr + 1) % memory.size()]) << 8);
    return value;
  } else {
    return parseImmediate(operand);
  }
}

void Simulator::setOperand(const std::string &operand, uint16_t value) {
  if (isRegister(operand)) {
    setRegisterValue(operand, value);
    return;
  } else if (isMemory(operand)) {
    uint16_t addr = parseMemoryAddress(operand);
    // Write 16-bit word to memory (little-endian)
    memory[addr] = static_cast<uint8_t>(value & 0xFF);
    memory[(addr + 1) % memory.size()] =
        static_cast<uint8_t>((value >> 8) & 0xFF);
    return;
  }

  throw std::runtime_error("setOperand() encountered unknown operand type");
}

void Simulator::setFlags(uint16_t result, bool carry, bool overflow) {
  handleZeroFlag(result);
  handleSignFlag(result);
  handleCarryFlag(carry);
  handleOverflowFlag(overflow);
  handleParityFlag(result);
}

int8_t Simulator::readDisplacement(uint32_t offset) {
  if (offset + 1 >= bytestream.size()) {
    throw std::runtime_error("Cannot read displacement: out of bounds");
  }
  return static_cast<int8_t>(bytestream[offset + 1]);
}

uint32_t Simulator::calculateJumpTarget(uint32_t currentIP,
                                        int8_t displacement) {
  // Target is current IP + 2 (opcode + displacement byte) + displacement
  int32_t target =
      static_cast<int32_t>(currentIP) + 2 + static_cast<int32_t>(displacement);

  // Ensure target stays within bytecode bounds
  if (target < 0 || target > static_cast<int32_t>(bytestream.size())) {
    throw std::runtime_error("Jump target out of bounds");
  }

  return static_cast<uint32_t>(target);
}

bool Simulator::shouldJump(const std::string &mnemonic) const {
  // Conditional jumps based on flags
  if (mnemonic == "JE" || mnemonic == "JZ") {
    return registers.flags.zero;
  } else if (mnemonic == "JNE" || mnemonic == "JNZ") {
    return !registers.flags.zero;
  } else if (mnemonic == "JB" || mnemonic == "JNAE" || mnemonic == "JC") {
    return registers.flags.carry;
  } else if (mnemonic == "JNB" || mnemonic == "JAE" || mnemonic == "JNC") {
    return !registers.flags.carry;
  } else if (mnemonic == "JL" || mnemonic == "JNGE") {
    return registers.flags.sign != registers.flags.overflow;
  } else if (mnemonic == "JNL" || mnemonic == "JGE") {
    return registers.flags.sign == registers.flags.overflow;
  } else if (mnemonic == "JLE" || mnemonic == "JNG") {
    return registers.flags.zero ||
           (registers.flags.sign != registers.flags.overflow);
  } else if (mnemonic == "JG" || mnemonic == "JNLE") {
    return !registers.flags.zero &&
           (registers.flags.sign == registers.flags.overflow);
  } else if (mnemonic == "JBE" || mnemonic == "JNA") {
    return registers.flags.carry || registers.flags.zero;
  } else if (mnemonic == "JA" || mnemonic == "JNBE") {
    return !registers.flags.carry && !registers.flags.zero;
  } else if (mnemonic == "JS") {
    return registers.flags.sign;
  } else if (mnemonic == "JNS") {
    return !registers.flags.sign;
  } else if (mnemonic == "JP" || mnemonic == "JPE") {
    return registers.flags.parity;
  } else if (mnemonic == "JNP" || mnemonic == "JPO") {
    return !registers.flags.parity;
  } else if (mnemonic == "JO") {
    return registers.flags.overflow;
  } else if (mnemonic == "JNO") {
    return !registers.flags.overflow;
  }
  // Unconditional jumps
  return true;
}

uint16_t &Simulator::getRegister(const std::string &regName) {
  // TODO: Return reference to the appropriate register
  // "AX" -> registers.ax
  // "BX" -> registers.bx
  // etc.
  //
  // For half-registers, this is trickier. Consider returning
  // a reference to the full register and handling shifts in the caller,
  // or use a helper function.

  throw std::runtime_error("getRegister() not implemented");
}

uint16_t Simulator::getRegisterValue(const std::string &regName) const {
  // Use member map for O(1) lookup - each instance has its own lambdas
  auto it = getRegisterMap.find(regName);
  if (it != getRegisterMap.end()) {
    return it->second();
  }
  throw std::runtime_error("Unknown register: " + regName);
}

uint16_t Simulator::parseImmediate(const std::string &immediate) const {
  try {
    // Check for hex format (0x prefix)
    if (immediate.size() > 2 && immediate[0] == '0' &&
        (immediate[1] == 'x' || immediate[1] == 'X')) {
      // Parse as hexadecimal
      return static_cast<uint16_t>(std::stoul(immediate, nullptr, 16));
    }

    // Parse as decimal (handles negative numbers)
    int32_t value = std::stoi(immediate);
    return static_cast<uint16_t>(value);
  } catch (const std::exception &e) {
    throw std::runtime_error("Failed to parse immediate value: " + immediate);
  }
}

void Simulator::setRegisterValue(const std::string &regName, uint16_t value) {
  // Use member map for O(1) lookup - each instance has its own lambdas
  auto it = setRegisterMap.find(regName);
  if (it != setRegisterMap.end()) {
    it->second(value);
    return;
  }
  throw std::runtime_error("Unknown register: " + regName);
}

bool Simulator::isRegister(const std::string &operand) const {
  static const std::set<std::string> registers = {
      "AX", "BX", "CX", "DX", "SP", "BP", "SI", "DI",
      "AL", "AH", "BL", "BH", "CL", "CH", "DL", "DH"};
  return registers.count(operand) > 0;
}

bool Simulator::isMemory(const std::string &operand) const {
  // Check for memory operand like "[5]" or with size prefix like "WORD [5]"
  std::string trimmed = operand;

  // Skip size prefix if present (e.g., "WORD [...]", "word [...]", "BYTE
  // [...]", "byte [...]")
  if (trimmed.size() >= 5) {
    std::string prefix = trimmed.substr(0, 5);
    // Convert to uppercase for comparison
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
    if (prefix == "WORD " || prefix == "BYTE ") {
      trimmed = trimmed.substr(5);
    }
  }

  return !trimmed.empty() && trimmed.front() == '[' && trimmed.back() == ']';
}

bool Simulator::isImmediate(const std::string &operand) const {
  return !isRegister(operand) && !isMemory(operand);
}

uint16_t Simulator::parseMemoryAddress(const std::string &operand) const {
  // Parse memory operand like "[5]", "[BX]", "[BX + SI]", "[BP - 4]"
  // or with size prefix like "word [5]", "byte [BP + 4]"

  std::string trimmed = operand;

  // Skip size prefix if present (uppercase or lowercase)
  if (trimmed.size() >= 5) {
    std::string prefix = trimmed.substr(0, 5);
    // Convert to uppercase for comparison
    std::transform(prefix.begin(), prefix.end(), prefix.begin(), ::toupper);
    if (prefix == "WORD " || prefix == "BYTE ") {
      trimmed = trimmed.substr(5);
    }
  }

  if (trimmed.empty() || trimmed.front() != '[' || trimmed.back() != ']') {
    throw std::runtime_error("Invalid memory operand: " + operand);
  }

  // Strip brackets
  std::string content = trimmed.substr(1, trimmed.length() - 2);

  // Trim whitespace
  auto trim = [](std::string s) {
    s.erase(0, s.find_first_not_of(" \t\r\n"));
    s.erase(s.find_last_not_of(" \t\r\n") + 1);
    return s;
  };
  content = trim(content);

  // Check for +/- operators
  size_t plusPos = content.find('+');
  size_t minusPos = content.find('-');

  // Find the rightmost +/- (to handle cases like "BX - 4")
  size_t opPos = std::string::npos;
  char opChar = ' ';
  if (plusPos != std::string::npos) {
    opPos = plusPos;
    opChar = '+';
  }
  if (minusPos != std::string::npos && minusPos > 0) {
    // Make sure it's not a leading minus
    if (opPos == std::string::npos || minusPos > opPos) {
      opPos = minusPos;
      opChar = '-';
    }
  }

  uint16_t address = 0;

  if (opPos == std::string::npos) {
    // Single operand: either a number or a register
    std::string op = trim(content);
    if (isRegister(op)) {
      address = getRegisterValue(op);
    } else {
      address = parseImmediate(op);
    }
  } else {
    // Two operands with +/-
    std::string left = trim(content.substr(0, opPos));
    std::string right = trim(content.substr(opPos + 1));

    uint16_t leftVal = 0;
    if (isRegister(left)) {
      leftVal = getRegisterValue(left);
    } else {
      leftVal = parseImmediate(left);
    }

    uint16_t rightVal = 0;
    if (isRegister(right)) {
      rightVal = getRegisterValue(right);
    } else {
      rightVal = parseImmediate(right);
    }

    if (opChar == '+') {
      address = leftVal + rightVal;
    } else {
      address = leftVal - rightVal;
    }
  }

  return address;
}
