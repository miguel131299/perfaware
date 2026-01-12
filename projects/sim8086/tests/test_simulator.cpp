#include <algorithm>
#include <cctype>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <sim8086/simulator.hpp>
#include <sim8086/utils.hpp>
#include <sstream>
#include <string>

struct SimulatorTestParameters {
  std::string filename;
  bool trackIPRegister = false;
  bool trackCycles = false;
};

// Normalize output for comparison: lowercase, remove Windows line endings,
// strip header line, and normalize whitespace
std::string normalizeSimulatorOutput(const std::string &input) {
  std::string normalized = input;

  // Convert to lowercase
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // Remove carriage returns (Windows line endings)
  normalized.erase(std::remove(normalized.begin(), normalized.end(), '\r'),
                   normalized.end());

  // Remove header line if present (starts with "---")
  size_t headerEnd = normalized.find('\n');
  if (headerEnd != std::string::npos && normalized.substr(0, 3) == "---") {
    normalized = normalized.substr(headerEnd + 1);
  }

  // Remove trailing spaces from each line
  std::istringstream iss(normalized);
  std::ostringstream oss;
  std::string line;
  bool lastLineWasEmpty = false;
  while (std::getline(iss, line)) {
    // Remove trailing whitespace
    while (!line.empty() && std::isspace(line.back())) {
      line.pop_back();
    }

    // Skip consecutive blank lines
    bool currentLineEmpty = line.empty();
    if (currentLineEmpty && lastLineWasEmpty) {
      continue;
    }
    lastLineWasEmpty = currentLineEmpty;

    oss << line << "\n";
  }

  std::string result = oss.str();

  // Normalize trailing newlines: trim all trailing newlines then add exactly
  // one
  while (!result.empty() && result.back() == '\n') {
    result.pop_back();
  }
  result += "\n";

  return result;
}

class SimulatorTest : public ::testing::TestWithParam<SimulatorTestParameters> {
};

TEST_P(SimulatorTest, ExecutionOutput) {
  const auto &param = GetParam();
  std::string baseDir =
      std::getenv("TEST_DATA_DIR") ? std::getenv("TEST_DATA_DIR") : ".";
  std::string binaryPath = baseDir + "/" + param.filename;
  std::string expectedPath = binaryPath + ".txt";

  try {
    // Read expected output
    std::ifstream expectedFile(expectedPath);
    if (!expectedFile) {
      GTEST_SKIP() << "No expected output file: " << expectedPath;
    }

    std::stringstream expectedBuffer;
    expectedBuffer << expectedFile.rdbuf();
    std::string expectedOutput = expectedBuffer.str();

    // Load and run simulator
    std::vector<char> bytecode = Utils::readBinaryFile(binaryPath);
    Simulator simulator(bytecode, param.trackIPRegister, param.trackCycles);
    simulator.run();

    // Get trace and state (with blank line separator)
    std::string actualOutput =
        simulator.getTrace() + "\n" + simulator.dumpState();

    // Compare output (case-insensitive)
    EXPECT_EQ(normalizeSimulatorOutput(actualOutput),
              normalizeSimulatorOutput(expectedOutput));
  } catch (const std::exception &ex) {
    FAIL() << "File: " << param.filename << " - Exception: " << ex.what();
  }
}

INSTANTIATE_TEST_SUITE_P(
    SimulatorExecution, SimulatorTest,
    ::testing::Values(
        SimulatorTestParameters{"listing_0043_immediate_movs"},
        SimulatorTestParameters{"listing_0044_register_movs"},
        SimulatorTestParameters{"listing_0046_add_sub_cmp"},
        SimulatorTestParameters{"listing_0048_ip_register", true},
        SimulatorTestParameters{"listing_0049_conditional_jumps", true},
        SimulatorTestParameters{"listing_0051_memory_mov", true},
        SimulatorTestParameters{"listing_0052_memory_add_loop", true},
        SimulatorTestParameters{"listing_0053_add_loop_challenge", true},
        SimulatorTestParameters{"listing_0056_estimating_cycles", true, true}));


