// main.cpp
#include <cstddef>
#include <exception>
#include <format>
#include <fstream>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

std::vector<char> readBinaryFile(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary);

  if (!file) {
    throw std::runtime_error("Could not open file: " + filename);
  }

  // Determine the file size
  file.seekg(0, std::ios::end);
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);

  // Read file into buffer
  std::vector<char> buffer(size);
  if (!file.read(buffer.data(), size)) {
    throw std::runtime_error("Error reading file: " + filename);
  }

  return buffer;
}

std::string getOpCode(unsigned char opcode) {
  opcode >>= 2;
  switch (opcode) {
  case 0b100010:
    return "MOV";

  default:
    throw std::runtime_error("Error decoding opcode: " +
                             std::to_string(opcode));
  }
}

bool isBitSet(char byte, size_t bitPosition) {
  return (byte & (1 << bitPosition)) != 0;
}

std::string getRegisterName(char regCode, bool isWFieldSet) {
  if (isWFieldSet) {
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

std::string getRegisterFromInstr(char byte, bool isRegRegister,
                                 bool isWFieldSet) {
  char regCode;
  if (isRegRegister) {
    regCode = (byte >> 3) & 0b111; // REG
  } else {
    regCode = byte & 0b111; // R/M
  }
  return getRegisterName(regCode, isWFieldSet);
}

std::string assembleInstructions(const std::vector<char> &bytestream) {

  // MOV instructions are 2 bytes long (at least the ones we consider)
  std::string instructions = "bits 16 \n\n";
  for (size_t i = 0; i + 1 < bytestream.size(); i += 2) {
    unsigned char firstByte = static_cast<unsigned char>(bytestream[i]);
    unsigned char secondByte = static_cast<unsigned char>(bytestream[i + 1]);

    std::string opcode = getOpCode(firstByte);
    bool isWFieldSet = isBitSet(firstByte, 0);
    bool isDFieldSet = isBitSet(firstByte, 1);
    std::string destReg =
        getRegisterFromInstr(secondByte, isDFieldSet, isWFieldSet);
    std::string srcReg =
        getRegisterFromInstr(secondByte, !isDFieldSet, isWFieldSet);

    instructions += std::format("{} {}, {}\n", opcode, destReg, srcReg);
  }

  return instructions;
}

int main(int argc, char *argv[]) {

  // Program also counts as argument, so we actually need 2 arguments
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <binary_file>\n";
    return 1;
  }

  const std::string filename = argv[1];

  try {
    std::vector<char> data = readBinaryFile(filename);
    std::string instructions = assembleInstructions(data);
    std::cout << instructions << std::endl;
  } catch (const std::exception &ex) {
    std::cerr << "Error: " << ex.what() << "\n";
    return 1;
  }

  return 0;
}
