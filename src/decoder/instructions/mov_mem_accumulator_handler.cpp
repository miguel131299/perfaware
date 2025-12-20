#include "mov_mem_accumulator_handler.hpp"

#include "../bit_utilities.hpp"
#include "../tables/registers.hpp"
#include "mov_utilities.hpp"
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
uint32_t MOVMemAccumulatorHandler::decode(std::stringstream &ss,
                                          const std::vector<char> &bytestream,
                                          uint32_t baseOffset) {
  // Opcode patterns:
  //   0xA0 (0b10100000): MOV AL, moffs8
  //   0xA1 (0b10100001): MOV AX, moffs16
  //
  // Byte format: [opcode][low address byte][high address byte]
  // W bit is encoded in the opcode (bit 0)

  bool isWordOperation = isBitSet(bytestream[baseOffset], BIT_POS_W);
  
  // Read the direct address (2 bytes, little-endian)
  int16_t address = readInt16(bytestream, baseOffset + 1);
  
  std::string memOperand = "[" + std::to_string(address) + "]";
  std::string regOperand = std::string(getRegisterName(0, isWordOperation)); // 0 = AX/AL
  
  outputMOVInstruction(ss, regOperand, memOperand);
  
  return 3; // opcode + 2 bytes for address
}
