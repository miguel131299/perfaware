#include "mov_accumulator_mem_handler.hpp"

#include "../bit_utilities.hpp"
#include "../tables/registers.hpp"
#include "mov_utilities.hpp"
#include <sstream>
#include <string>

//-----------------------------------------------------------------------------
uint32_t MOVAccumulatorMemHandler::decode(std::stringstream &ss,
                                          const std::vector<char> &bytestream,
                                          uint32_t baseOffset) {
  // Opcode patterns:
  //   0xA2 (0b10100010): MOV moffs8, AL
  //   0xA3 (0b10100011): MOV moffs16, AX
  //
  // Byte format: [opcode][low address byte][high address byte]
  // W bit is encoded in the opcode (bit 0)

  bool isWordOperation = isBitSet(bytestream[baseOffset], BIT_POS_W);
  
  // Read the direct address (2 bytes, little-endian)
  int16_t address = readInt16(bytestream, baseOffset + 1);
  
  std::string memOperand = "[" + std::to_string(address) + "]";
  std::string regOperand = std::string(getRegisterName(0, isWordOperation)); // 0 = AX/AL
  
  outputMOVInstruction(ss, memOperand, regOperand);
  
  return 3; // opcode + 2 bytes for address
}
