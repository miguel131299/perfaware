#include <bitset>
#include <cstdint>
#include <format>
#include <sim8086/decoder.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
//-----------------------------------------------------------------------------
// Named constants for bit field extraction
constexpr uint8_t BIT_FIELD_MOD_MASK = 0b11000000;
constexpr uint8_t BIT_FIELD_REG_MASK = 0b00111000;
constexpr uint8_t BIT_FIELD_RM_MASK = 0b00000111;
// Bit positions for single-bit fields
constexpr size_t BIT_POS_D = 1;  // Direction bit
constexpr size_t BIT_POS_W = 0;  // Word/Byte bit
// Register field encoded directly in MOV immediate-to-register opcode byte
constexpr uint8_t MOV_IMM_REG_MASK = 0b00000111;
//-----------------------------------------------------------------------------
// Register lookup tables
static constexpr std::array<std::string_view, 8> WORD_REGISTERS = {
    "AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI"};

static constexpr std::array<std::string_view, 8> BYTE_REGISTERS = {
    "AL", "CL", "DL", "BL", "AH", "CH", "DH", "BH"};

// Helper functions for reading immediates
static int16_t read16BitSigned(const std::vector<char> &bytestream,
                               uint32_t offset) {
  uint8_t lowByte = static_cast<uint8_t>(bytestream[offset]);
  uint8_t highByte = static_cast<uint8_t>(bytestream[offset + 1]);
  return lowByte | (static_cast<int16_t>(highByte) << 8);
}

static int8_t read8BitSigned(const std::vector<char> &bytestream,
                             uint32_t offset) {
  return static_cast<int8_t>(bytestream[offset]);
}

//-----------------------------------------------------------------------------
enum class MODEncoding : uint8_t {
  MEMORY_MODE_NO_DISPLACEMENT,
  MEMORY_MODE_8_DISPLACEMENT,
  MEMORY_MODE_16_DISPLACEMENT,
  REGISTER_MODE,
};
//-----------------------------------------------------------------------------
enum class InstructionEncoding : uint8_t {
  // MOV Instructions
  MOV_REGMEM_TOFROM_REG,
  MOV_IMM_TO_REG,
  MOV_MEM_TO_ACCUM,
  MOV_ACCUM_TO_MEM,
};

static std::string InstructionToMnemonic(const InstructionEncoding instr) {
  switch (instr) {
  case InstructionEncoding::MOV_REGMEM_TOFROM_REG:
  case InstructionEncoding::MOV_IMM_TO_REG:
  case InstructionEncoding::MOV_MEM_TO_ACCUM:
  case InstructionEncoding::MOV_ACCUM_TO_MEM:
    return "MOV";
  default:
    throw std::runtime_error("Error converting instruction to Mnemonic: " +
                             std::to_string(static_cast<int>(instr)));
  }
}
//-----------------------------------------------------------------------------
static MODEncoding getMODEncoding(uint8_t byte) {
  byte = (byte & BIT_FIELD_MOD_MASK) >> 6;
  switch (byte) {
  case 0:
    return MODEncoding::MEMORY_MODE_NO_DISPLACEMENT;
  case 1:
    return MODEncoding::MEMORY_MODE_8_DISPLACEMENT;
  case 2:
    return MODEncoding::MEMORY_MODE_16_DISPLACEMENT;
  case 3:
    return MODEncoding::REGISTER_MODE;
  default:
    throw std::runtime_error("Error decoding MOD: " + std::to_string(byte));
  }
}
//-----------------------------------------------------------------------------
static InstructionEncoding getInstrEncoding(uint8_t byte) {
  //---------MOV Instructions----------------------

  // Register/memory to/from register
  if ((byte & 0b11111100) == 0b10001000) {
    return InstructionEncoding::MOV_REGMEM_TOFROM_REG;
  }

  // Immediate to register
  if ((byte & 0b11110000) == 0b10110000) {
    return InstructionEncoding::MOV_IMM_TO_REG;
  }

  // Memory to accumulator
  if ((byte & 0b11111110) == 0b10100000) {
    return InstructionEncoding::MOV_MEM_TO_ACCUM;
  }

  // Accumulator to memory
  if ((byte & 0b11111110) == 0b10100010) {
    return InstructionEncoding::MOV_ACCUM_TO_MEM;
  }

  throw std::runtime_error("Error decoding instruction: " +
                           std::to_string(byte));
}
//-----------------------------------------------------------------------------
static std::string getRegisterFieldEncoding(char regCode,
                                            bool isWordOperation) {
  if (regCode < 0 || regCode > 7) {
    throw std::runtime_error("Invalid register code: " +
                             std::to_string(regCode));
  }

  auto &registers = isWordOperation ? WORD_REGISTERS : BYTE_REGISTERS;
  return std::string(registers[regCode]);
}
//-----------------------------------------------------------------------------
std::string getRegisterFromRegOrRM(char byte, bool getReg,
                                   bool isWordOperation) {
  char regCode;
  if (getReg) {
    regCode = (byte & BIT_FIELD_REG_MASK) >> 3;  // REG
  } else {
    regCode = byte & BIT_FIELD_RM_MASK;  // R/M
  }
  return getRegisterFieldEncoding(regCode, isWordOperation);
}
//-----------------------------------------------------------------------------
std::string getEffectiveAddressRegisters(uint8_t RM,
                                         const std::vector<char> &bytestream,
                                         const uint32_t base,
                                         bool isWithDisplacement,
                                         bool &isDirectAddress) {
  switch (RM) {
  case 0:
    return "BX + SI";
  case 1:
    return "BX + DI";
  case 2:
    return "BP + SI";
  case 3:
    return "BP + DI";
  case 4:
    return "SI";
  case 5:
    return "DI";
  case 6: {
    if (isWithDisplacement) {
      return "BP";
    } else {
      // DIRECT ADDRESS
      uint8_t lowByte =
          static_cast<uint8_t>(bytestream[base + 2]); // third byte
      uint8_t highByte =
          static_cast<uint8_t>(bytestream[base + 3]); // fourth byte
      int16_t value = lowByte | (static_cast<int16_t>(highByte) << 8);
      std::string address = std::to_string(value);
      isDirectAddress = true;
      return address;
    }
  }
  case 7:
    return "BX";
  }
  throw std::runtime_error("Error getting effective address registers: " +
                           std::to_string(RM));
}
//-----------------------------------------------------------------------------
std::string getEffectiveAddressRegistersWithDisplacement(
    uint8_t RM, const std::vector<char> &bytestream, const uint32_t base,
    bool is16BitDisplacement) {

  std::string displacement;
  if (is16BitDisplacement) {
    // Word operation. 16 bit immediate (signed)
    int16_t value = read16BitSigned(bytestream, base + 2);
    displacement = std::to_string(value);
  } else {
    // Byte operation. 8 bit immediate (signed)
    int8_t value = read8BitSigned(bytestream, base + 2);
    displacement = std::to_string(value);
  }

  // TODO: is there a better way to do this than pass a reference?
  bool isDirectAddress = false;
  std::string registers =
      getEffectiveAddressRegisters(RM, bytestream, base, true, isDirectAddress);

  // If displacement is 0, just ignore it.
  std::string result =
      registers + (displacement == "0" ? "" : " + " + displacement);
  return "[" + result + "]";
}

//-----------------------------------------------------------------------------
static void outputInstruction(std::stringstream &ss,
                              const InstructionEncoding instr,
                              bool isRegDestination,
                              const std::string &regOperand,
                              const std::string &memOperand) {
  ss << std::format("{} {}, {}\n", InstructionToMnemonic(instr),
                    isRegDestination ? regOperand : memOperand,
                    isRegDestination ? memOperand : regOperand);
}

static bool isBitSet(char byte, size_t bitPosition) {
  return (byte & (1 << bitPosition)) != 0;
}
//-----------------------------------------------------------------------------
static uint32_t handle_MOV_REGMEM_TOFROM_REG_REGISTER_MODE(
    std::stringstream &ss, uint8_t secondByte, bool isRegDestination,
    bool isWordOperation) {
  std::string destReg =
      getRegisterFromRegOrRM(secondByte, isRegDestination, isWordOperation);
  std::string srcReg =
      getRegisterFromRegOrRM(secondByte, !isRegDestination, isWordOperation);

  ss << std::format(
      "{} {}, {}\n",
      InstructionToMnemonic(InstructionEncoding::MOV_REGMEM_TOFROM_REG),
      destReg, srcReg);

  return 2;
}

static uint32_t handle_MOV_REGMEM_TOFROM_REG_MEMORY_MODE_NO_DISPLACEMENT(
    std::stringstream &ss, const std::vector<char> &bytestream,
    const uint32_t base, bool isRegDestination, bool isWordOperation) {

  std::string regName =
      getRegisterFromRegOrRM(bytestream[base + 1], true, isWordOperation);

  uint8_t RM = bytestream[base + 1] & BIT_FIELD_RM_MASK;

  bool isDirectAddress = false;

  std::string effectiveAddressRegisters =
      "[" +
      getEffectiveAddressRegisters(RM, bytestream, base, false,
                                   isDirectAddress) +
      "]";

  outputInstruction(ss, InstructionEncoding::MOV_REGMEM_TOFROM_REG,
                    isRegDestination, regName, effectiveAddressRegisters);

  // If we write direct address directly, we read 2 more bytes
  return isDirectAddress ? 4 : 2;
}

static uint32_t handle_MOV_REGMEM_TOFROM_REG_MEMORY_MODE_WITH_DISPLACEMENT(
    std::stringstream &ss, const std::vector<char> &bytestream,
    const uint32_t base, bool isRegDestination, bool isWordOperation,
    bool is16BitDisplacement) {

  std::string regName =
      getRegisterFromRegOrRM(bytestream[base + 1], true, isWordOperation);

  uint8_t RM = bytestream[base + 1] & BIT_FIELD_RM_MASK;

  std::string effectiveAddressRegisters =
      getEffectiveAddressRegistersWithDisplacement(RM, bytestream, base,
                                                   is16BitDisplacement);

  outputInstruction(ss, InstructionEncoding::MOV_REGMEM_TOFROM_REG,
                    isRegDestination, regName, effectiveAddressRegisters);

  // 8 bit displacement -> 3 bytes, 16 bit displacement -> 4 bytes
  return is16BitDisplacement ? 4 : 3;
}

//-----------------------------------------------------------------------------
static uint32_t
handle_MOV_REGMEM_TOFROM_REG(std::stringstream &ss,
                             const std::vector<char> &bytestream,
                             const uint32_t base) {

  // Word/Byte Operation
  bool isWFieldSet = isBitSet(bytestream[base], BIT_POS_W);

  // Direction. D = 0 -> REG is source operand. D = 1 -> REG is destination
  // operand.
  bool isDFieldSet = isBitSet(bytestream[base], BIT_POS_D);

  unsigned char secondByte = static_cast<unsigned char>(bytestream[base + 1]);

  MODEncoding mod = getMODEncoding(secondByte);

  switch (mod) {
  case MODEncoding::MEMORY_MODE_NO_DISPLACEMENT:
    return handle_MOV_REGMEM_TOFROM_REG_MEMORY_MODE_NO_DISPLACEMENT(
        ss, bytestream, base, isDFieldSet, isWFieldSet);
  case MODEncoding::MEMORY_MODE_8_DISPLACEMENT:
    return handle_MOV_REGMEM_TOFROM_REG_MEMORY_MODE_WITH_DISPLACEMENT(
        ss, bytestream, base, isDFieldSet, isWFieldSet, false);
  case MODEncoding::MEMORY_MODE_16_DISPLACEMENT:
    return handle_MOV_REGMEM_TOFROM_REG_MEMORY_MODE_WITH_DISPLACEMENT(
        ss, bytestream, base, isDFieldSet, isWFieldSet, true);
  case MODEncoding::REGISTER_MODE:
    return handle_MOV_REGMEM_TOFROM_REG_REGISTER_MODE(ss, secondByte,
                                                      isDFieldSet, isWFieldSet);
    break;
  }
}
//-----------------------------------------------------------------------------
static uint32_t handle_MOV_IMM_TO_REG(std::stringstream &ss,
                                      const std::vector<char> &bytestream,
                                      const uint32_t base) {
  bool isWFieldSet = isBitSet(bytestream[base], 3);

  // In MOV immediate to register, the register code is in bits 2-0 of the opcode byte
  uint8_t registerCode = bytestream[base] & MOV_IMM_REG_MASK;

  std::string destReg = getRegisterFieldEncoding(registerCode, isWFieldSet);

  std::string immediateValue;
  uint32_t bytesRead;

  if (isWFieldSet) {
    // Word operation. 16 bit immediate (signed)
    int16_t value = read16BitSigned(bytestream, base + 1);
    immediateValue = std::to_string(value);
    bytesRead = 3;
  } else {
    // Byte operation. 8 bit immediate (signed)
    int8_t value = read8BitSigned(bytestream, base + 1);
    immediateValue = std::to_string(value);
    bytesRead = 2;
  }

  ss << std::format("{} {}, {}\n",
                    InstructionToMnemonic(InstructionEncoding::MOV_IMM_TO_REG),
                    destReg, immediateValue);

  return bytesRead;
}
//-----------------------------------------------------------------------------
static inline uint32_t
dispatchToInstrHandler(std::stringstream &ss,
                       const std::vector<char> &bytestream,
                       const uint32_t base) {
  InstructionEncoding instr = getInstrEncoding(bytestream[base]);

  switch (instr) {
  case InstructionEncoding::MOV_REGMEM_TOFROM_REG:
    return handle_MOV_REGMEM_TOFROM_REG(ss, bytestream, base);
  case InstructionEncoding::MOV_IMM_TO_REG:
    return handle_MOV_IMM_TO_REG(ss, bytestream, base);

  default:
    throw std::runtime_error(
        "Error decoding register: " +
        std::to_string(static_cast<int>(bytestream[base])) + " (0b" +
        std::bitset<8>(bytestream[base]).to_string() + ")");
  }
}
//-----------------------------------------------------------------------------
std::string Decoder::assembleInstructions(const std::vector<char> &bytestream) {
  // add prefix to tell assembler we are using 8086
  std::stringstream ss;
  ss << "bits 16\n\n";

  // MOV instructions are 2 bytes long (at least the ones we consider)
  uint32_t i = 0;
  while (i < bytestream.size()) {
    i += dispatchToInstrHandler(ss, bytestream, i);
  }

  return ss.str();
}