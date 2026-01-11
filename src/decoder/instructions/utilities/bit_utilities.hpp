#pragma once

#include <cstdint>
#include <vector>

//-----------------------------------------------------------------------------
// Named constants for bit field extraction
constexpr uint8_t BIT_FIELD_MOD_MASK = 0b11000000;
constexpr uint8_t BIT_FIELD_REG_MASK = 0b00111000;
constexpr uint8_t BIT_FIELD_RM_MASK = 0b00000111;

// Bit positions for single-bit fields
constexpr std::size_t BIT_POS_D = 1; // Direction bit
constexpr std::size_t BIT_POS_W = 0; // Word/Byte bit

// Register field encoded directly in MOV immediate-to-register opcode byte
constexpr uint8_t MOV_IMM_REG_MASK = 0b00000111;

//-----------------------------------------------------------------------------
// Utility functions
inline bool isBitSet(char byte, std::size_t bitPosition) {
  return (byte & (1 << bitPosition)) != 0;
}

inline uint8_t extractBits(uint8_t byte, uint8_t mask, std::size_t shift = 0) {
  return (byte & mask) >> shift;
}

// Read two bytes from bytes at given offset and combine into 16-bit signed
// integer
inline int16_t readInt16(const std::vector<uint8_t> &bytes, uint32_t offset) {
  uint8_t lowByte = bytes[offset];
  uint8_t highByte = bytes[offset + 1];
  return lowByte | (static_cast<int16_t>(highByte) << 8);
}
