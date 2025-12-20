#include <bitset>
#include <cstdint>
#include <format>
#include <sim8086/decoder.hpp>
#include <sstream>
#include <stdexcept>
#include <string>
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
  byte >>= 6;
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
  if (isWordOperation) {
    // Word Operation
    switch (regCode) {
    case 0b000:
      return "AX";
    case 0b001:
      return "CX";
    case 0b010:
      return "DX";
    case 0b011:
      return "BX";
    case 0b100:
      return "SP";
    case 0b101:
      return "BP";
    case 0b110:
      return "SI";
    case 0b111:
      return "DI";
    }
  } else {
    // Byte Operation
    switch (regCode) {
    case 0b000:
      return "AL";
    case 0b001:
      return "CL";
    case 0b010:
      return "DL";
    case 0b011:
      return "BL";
    case 0b100:
      return "AH";
    case 0b101:
      return "CH";
    case 0b110:
      return "DH";
    case 0b111:
      return "BH";
    }
  }
  throw std::runtime_error("Error decoding register: " +
                           std::to_string(regCode));
}
//-----------------------------------------------------------------------------
std::string getRegisterFromRegOrRM(char byte, bool getReg,
                                   bool isWordOperation) {
  char regCode;
  if (getReg) {
    regCode = (byte >> 3) & 0b111; // REG
  } else {
    regCode = byte & 0b111; // R/M
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
    uint8_t lowByte = static_cast<int8_t>(bytestream[base + 2]);
    uint8_t highByte = static_cast<int8_t>(bytestream[base + 3]);
    int16_t value = lowByte | (highByte << 8);
    displacement = std::to_string(value);
  } else {
    // Byte operation. 8 bit immediate (signed)
    int8_t value = static_cast<int8_t>(bytestream[base + 2]);
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

  uint8_t RM = bytestream[base + 1] & 0b111;

  bool isDirectAddress = false;

  std::string effectiveAddressRegisters =
      "[" +
      getEffectiveAddressRegisters(RM, bytestream, base, false,
                                   isDirectAddress) +
      "]";

  ss << std::format(
      "{} {}, {}\n",
      InstructionToMnemonic(InstructionEncoding::MOV_REGMEM_TOFROM_REG),
      isRegDestination ? regName : effectiveAddressRegisters,
      isRegDestination ? effectiveAddressRegisters : regName);

  // If we write direct address directly, we read 2 more bytes
  return isDirectAddress ? 4 : 2;
}

static uint32_t handle_MOV_REGMEM_TOFROM_REG_MEMORY_MODE_WITH_DISPLACEMENT(
    std::stringstream &ss, const std::vector<char> &bytestream,
    const uint32_t base, bool isRegDestination, bool isWordOperation,
    bool is16BitDisplacement) {

  std::string regName =
      getRegisterFromRegOrRM(bytestream[base + 1], true, isWordOperation);

  uint8_t RM = bytestream[base + 1] & 0b111;

  std::string effectiveAddressRegisters =
      getEffectiveAddressRegistersWithDisplacement(RM, bytestream, base,
                                                   is16BitDisplacement);

  ss << std::format(
      "{} {}, {}\n",
      InstructionToMnemonic(InstructionEncoding::MOV_REGMEM_TOFROM_REG),
      isRegDestination ? regName : effectiveAddressRegisters,
      isRegDestination ? effectiveAddressRegisters : regName);

  // 8 bit displacement -> 3 bytes, 16 bit displacement -> 4 bytes
  return is16BitDisplacement ? 4 : 3;
}

//-----------------------------------------------------------------------------
static uint32_t
handle_MOV_REGMEM_TOFROM_REG(std::stringstream &ss,
                             const std::vector<char> &bytestream,
                             const uint32_t base) {

  // Word/Byte Operation
  bool isWFieldSet = isBitSet(bytestream[base], 0);

  // Direction. D = 0 -> REG is source operand. D = 1 -> REG is destination
  // operand.
  bool isDFieldSet = isBitSet(bytestream[base], 1);

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

  uint8_t registerCode = bytestream[base] & 0b00000111;

  std::string destReg = getRegisterFieldEncoding(registerCode, isWFieldSet);

  uint32_t bytesRead;
  std::string immediateValue;

  if (isWFieldSet) {
    // Word operation. 16 bit immediate (signed)
    uint8_t lowByte = static_cast<int8_t>(bytestream[base + 1]);
    uint8_t highByte = static_cast<int8_t>(bytestream[base + 2]);
    int16_t value = lowByte | (highByte << 8);
    immediateValue = std::to_string(value);
    bytesRead = 3;
  } else {
    // Byte operation. 8 bit immediate (signed)
    int8_t value = static_cast<int8_t>(bytestream[base + 1]);
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