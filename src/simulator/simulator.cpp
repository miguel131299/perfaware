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

Simulator::Simulator(const std::vector<char> &bytestream)
    : memory(65536, 0), bytestream(bytestream) {
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
      {"AH", [this](uint16_t v) { registers.ax = (registers.ax & 0xFF) | ((v & 0xFF) << 8); }},
      {"BH", [this](uint16_t v) { registers.bx = (registers.bx & 0xFF) | ((v & 0xFF) << 8); }},
      {"CH", [this](uint16_t v) { registers.cx = (registers.cx & 0xFF) | ((v & 0xFF) << 8); }},
      {"DH", [this](uint16_t v) { registers.dx = (registers.dx & 0xFF) | ((v & 0xFF) << 8); }},
      {"AL", [this](uint16_t v) { registers.ax = (registers.ax & 0xFF00) | (v & 0xFF); }},
      {"BL", [this](uint16_t v) { registers.bx = (registers.bx & 0xFF00) | (v & 0xFF); }},
      {"CL", [this](uint16_t v) { registers.cx = (registers.cx & 0xFF00) | (v & 0xFF); }},
      {"DL", [this](uint16_t v) { registers.dx = (registers.dx & 0xFF00) | (v & 0xFF); }},
  };

  decodeAllInstructions();
}

void Simulator::run() {
  // TODO: Continuously step() until we reach end of instructions or hit a halt
  while (currentInstructionIndex < instructions.size()) {
    step();
  }
}

void Simulator::step() {
  if (currentInstructionIndex >= instructions.size()) {
    return; // End of program
  }

  const DecodedInstruction &decodedInstr =
      instructions[currentInstructionIndex];

  // Capture old register value before execution (for MOV operations)
  uint16_t oldValue = 0;
  bool trackRegister = false;
  std::string regName;

  if (isRegister(decodedInstr.parsed.destOp)) {
    regName = decodedInstr.parsed.destOp;
    oldValue = getRegisterValue(regName);
    trackRegister = true;
  }

  // Execute instruction
  executeInstruction(decodedInstr.parsed);

  // Output trace
  traceOutput << decodedInstr.decoded;
  if (trackRegister) {
    uint16_t newValue = getRegisterValue(regName);
    traceOutput << " ; " << regName << ":0x" << std::hex << oldValue << "->0x"
                << newValue << std::dec;
  }
  traceOutput << "\n";

  currentInstructionIndex++;
}

void Simulator::decodeAllInstructions() {
  // Decode all instructions at once
  std::string allDecoded = Decoder::assembleInstructions(bytestream);

  // Parse each line as an instruction
  std::istringstream iss(allDecoded);
  std::string line;
  uint32_t byteOffset = 0;

  while (std::getline(iss, line)) {
    if (line.empty())
      continue;

    // Skip non-instruction directives (like "bits 16")
    if (line.find("bits") != std::string::npos ||
        line.find("Bits") != std::string::npos)
      continue;

    // Parse instruction
    Instruction parsed = parseInstruction(line);

    // Store decoded instruction
    instructions.push_back(DecodedInstruction{byteOffset, line, parsed});

    // TODO: Calculate actual byte offset from parsing the instruction
    // For now, assume average instruction length of 3 bytes
    byteOffset += 3;
  }
}

std::string Simulator::dumpState() const {
  std::ostringstream oss;
  oss << "Final registers:\n";

  // Format each 16-bit register
  oss << "      ax: 0x" << std::hex << std::setfill('0') << std::setw(4)
      << registers.ax << " (" << std::dec << registers.ax << ")\n";
  oss << "      bx: 0x" << std::hex << std::setfill('0') << std::setw(4)
      << registers.bx << " (" << std::dec << registers.bx << ")\n";
  oss << "      cx: 0x" << std::hex << std::setfill('0') << std::setw(4)
      << registers.cx << " (" << std::dec << registers.cx << ")\n";
  oss << "      dx: 0x" << std::hex << std::setfill('0') << std::setw(4)
      << registers.dx << " (" << std::dec << registers.dx << ")\n";
  oss << "      sp: 0x" << std::hex << std::setfill('0') << std::setw(4)
      << registers.sp << " (" << std::dec << registers.sp << ")\n";
  oss << "      bp: 0x" << std::hex << std::setfill('0') << std::setw(4)
      << registers.bp << " (" << std::dec << registers.bp << ")\n";
  oss << "      si: 0x" << std::hex << std::setfill('0') << std::setw(4)
      << registers.si << " (" << std::dec << registers.si << ")\n";
  oss << "      di: 0x" << std::hex << std::setfill('0') << std::setw(4)
      << registers.di << " (" << std::dec << registers.di << ")\n";

  return oss.str();
}

std::string Simulator::getTrace() const { return traceOutput.str(); }

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

void Simulator::executeInstruction(const Instruction &instr) {
  // TODO: Dispatch to appropriate executor based on mnemonic
  if (instr.mnemonic == "MOV") {
    executeMov(instr.destOp, instr.srcOp);
  } else if (instr.mnemonic == "ADD") {
    executeAdd(instr.destOp, instr.srcOp);
  } else if (instr.mnemonic == "SUB") {
    executeSub(instr.destOp, instr.srcOp);
  } else if (instr.mnemonic == "CMP") {
    executeCmp(instr.destOp, instr.srcOp);
  } else if (instr.mnemonic.find('J') == 0) { // All jumps start with 'j'
    executeJump(instr.mnemonic);
  }
  // Add more instruction types as needed
}

void Simulator::executeMov(const std::string &dest, const std::string &src) {
  // 1. Resolve source operand to a value
  uint16_t srcVal = resolveOperand(src);
  // 2. Set destination operand to that value
  setOperand(dest, srcVal);
}

void Simulator::executeAdd(const std::string &dest, const std::string &src) {
  // TODO: ADD dest, src
  // 1. Resolve both operands
  // 2. Add them: result = dest + src
  // 3. Set destination to result
  // 4. Update flags based on result
  // 5. Handle overflow/carry detection

  throw std::runtime_error("executeAdd() not implemented");
}

void Simulator::executeSub(const std::string &dest, const std::string &src) {
  // TODO: SUB dest, src
  // Similar to ADD but: result = dest - src
  // Update flags appropriately

  throw std::runtime_error("executeSub() not implemented");
}

void Simulator::executeCmp(const std::string &dest, const std::string &src) {
  // TODO: CMP dest, src
  // Like SUB but only updates flags, doesn't store result
  // result = dest - src (for flags only, not stored)

  throw std::runtime_error("executeCmp() not implemented");
}

void Simulator::executeJump(const std::string &mnemonic) {
  // TODO: Handle jumps (JE, JNZ, JL, LOOP, etc.)
  // 1. Check if jump condition is met
  // 2. If yes: read displacement from next byte and update IP
  // 3. If no: just advance IP normally
  //
  // The displacement is a signed 8-bit value after the opcode
  // Target = IP + 2 + displacement

  throw std::runtime_error("executeJump() not implemented");
}

uint16_t Simulator::resolveOperand(const std::string &operand) {
  // TODO: Convert operand string to actual value
  // Cases:
  // - Register: "AX" -> registers.ax
  // - Memory direct: "[5]" -> memory[5]
  // - Memory reg: "[BX]" -> memory[registers.bx]
  // - Memory reg+reg: "[BX + SI]" -> memory[registers.bx + registers.si]
  // - Memory reg+offset: "[BP + 4]" -> memory[registers.bp + 4]
  // - Immediate: "5", "-30", "0x1234" -> parse as number
  // - Register halves: "AL", "AH", "BL", etc.

  // if (operand[0] == '[' && operand[operand.length()-1] == ']') {
  //   return memory[]
  // }
  if (isRegister(operand)) {
    return getRegisterValue(operand);
  }
  // else if (isMemory(operand)) {
  //   uint16_t addr = parseMemoryAddress(operand);
  // }
  else {
    return parseImmediate(operand);
  }

  throw std::runtime_error("resolveOperand() not implemented");
}

void Simulator::setOperand(const std::string &operand, uint16_t value) {
  // - "AX" -> registers.ax = value
  // - "[BX]" -> memory[registers.bx] = value (handle byte/word)
  // - "AL" -> lower byte of AX
  // - "AH" -> upper byte of AX
  if (isRegister(operand)) {
    setRegisterValue(operand, value);
    return;
  }
  // else if (isMemory(operand)) {
  //   uint16_t addr = parseMemoryAddress(operand);
  //   memory[addr] = value;
  // }

  throw std::runtime_error("setOperand() not fully implemented");
}

void Simulator::setFlags(uint16_t result, bool carry, bool overflow) {
  // TODO: Update flag register
  // - Zero flag: set if result == 0
  // - Sign flag: set if MSB (bit 15) is 1
  // - Carry flag: already provided as parameter
  // - Overflow flag: already provided as parameter
  // - Parity flag: set if lower 8 bits has even number of 1s

  throw std::runtime_error("setFlags() not implemented");
}

bool Simulator::shouldJump(const std::string &mnemonic) const {
  // TODO: Check if a conditional jump should be taken
  // Map mnemonics to flag conditions:
  // - "je" (jump if equal): zero flag set
  // - "jnz" (jump if not zero): zero flag clear
  // - "jl" (jump if less): sign flag != overflow flag
  // - "jb" (jump if below): carry flag set
  // - "js" (jump if sign): sign flag set
  // - etc.
  //
  // For unconditional jumps (jmp, loop, loopz, etc.), return true

  throw std::runtime_error("shouldJump() not implemented");
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
  return !operand.empty() && operand.front() == '[' && operand.back() == ']';
}

bool Simulator::isImmediate(const std::string &operand) const {
  return !isRegister(operand) && !isMemory(operand);
}
