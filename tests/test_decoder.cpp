#include "gtest/gtest.h"
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <sim8086/decoder.hpp>
#include <sim8086/utils.hpp>
#include <string>

struct DecoderTestParameters {
  std::string filename;
};

class DecoderTest : public ::testing::TestWithParam<DecoderTestParameters> {};

std::string normalizeAssembly(const std::string &input) {
  std::istringstream stream(input);
  std::ostringstream normalized;
  std::string line;

  while (std::getline(stream, line)) {
    // Remove comments (everything after ;)
    auto commentPos = line.find(';');
    if (commentPos != std::string::npos) {
      line = line.substr(0, commentPos);
    }

    // Trim leading and trailing whitespace
    line.erase(0, line.find_first_not_of(" \t\r\n"));
    line.erase(line.find_last_not_of(" \t\r\n") + 1);

    if (!line.empty()) {
      // Convert to lowercase
      std::transform(line.begin(), line.end(), line.begin(),
                     [](unsigned char c) { return std::tolower(c); });

      normalized << line << '\n';
    }
  }

  return normalized.str();
}

TEST_P(DecoderTest, InstructionDecoding) {
  const auto &param = GetParam();
  std::string baseDir =
      std::getenv("TEST_DATA_DIR") ? std::getenv("TEST_DATA_DIR") : ".";
  std::string fullPath = baseDir + "/" + param.filename;

  // Step 1: Decode binary file
  std::vector<char> data = Utils::readBinaryFile(fullPath);
  std::string decoded = Decoder::assembleInstructions(data);

  // Step 2: Read reference .asm file
  std::ifstream refFile(fullPath + ".asm");
  ASSERT_TRUE(refFile) << "Failed to open reference file: " << param.filename
                       << ".asm";

  std::stringstream buffer;
  buffer << refFile.rdbuf();
  std::string expected = buffer.str();

  // Step 3: Compare
  std::string normalizedDecoded = normalizeAssembly(decoded);
  std::string normalizedExpected = normalizeAssembly(expected);

  EXPECT_EQ(normalizedDecoded, normalizedExpected);
}

INSTANTIATE_TEST_SUITE_P(
    MovDecoding, DecoderTest,
    ::testing::Values(DecoderTestParameters{"listing_0037_single_register_mov"},
                      DecoderTestParameters{"listing_0038_many_register_mov"}));