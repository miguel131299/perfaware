#include "haversine/processor.hpp"
#include "haversine/profiler.hpp"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

static constexpr double EARTH_RADIUS_KM = 6371.0;

double HaversineProcessor::degreesToRadians(double degrees) {
  return degrees * M_PI / 180.0;
}

static double square(double A) {
  double Result = (A * A);
  return Result;
}

double HaversineProcessor::computeHaversine(const Point &p0, const Point &p1) {
  TIME_BLOCK("ComputeHaversine");
  double lat0 = degreesToRadians(p0.y);
  double lon0 = degreesToRadians(p0.x);
  double lat1 = degreesToRadians(p1.y);
  double lon1 = degreesToRadians(p1.x);

  double dLat = lat1 - lat0;
  double dLon = lon1 - lon0;

  double a = square(std::sin(dLat / 2.0)) +
             std::cos(lat0) * std::cos(lat1) * square(std::sin(dLon / 2.0));

  double c = 2.0 * std::asin(std::sqrt(a));

  return EARTH_RADIUS_KM * c;
}

void HaversineProcessor::parseJSON(const std::string &filename) {
  // Use the split functions to avoid code duplication
  std::string jsonContent = readJSONFile(filename);
  parseJSONString(jsonContent);
}

std::string HaversineProcessor::readJSONFile(const std::string &filename) {
  TIME_BLOCK("ReadJSONFile");
  std::ifstream file(filename, std::ios::binary);

  if (!file) {
    throw std::runtime_error("Could not open file: " + filename);
  }

  // Read entire file into string
  std::string content;
  {
    TIME_BLOCK("ReadingIntoBuffer");
    content = std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
  }
  return content;
}

void HaversineProcessor::parseJSONString(const std::string &jsonContent) {
  TIME_BLOCK("ParseJSONString");
  // Parse from a string stream
  std::istringstream iss(jsonContent);
  Parser p(iss);
  pairs = p.parse_document();
}

double HaversineProcessor::readBinaryReference(const std::string &filename) {
  TIME_BLOCK("ReadBinaryReference");
  std::ifstream file(filename, std::ios::binary);
  if (!file) {
    throw std::runtime_error("Could not open binary file: " + filename);
  }

  // Seek to end and get file size
  file.seekg(0, std::ios::end);
  std::streampos fileSize = file.tellg();

  if (fileSize < (std::streampos)sizeof(double)) {
    throw std::runtime_error("Binary file too small");
  }

  // Seek to last double and read it
  file.seekg(-sizeof(double), std::ios::end);
  double referenceAverage;
  if (!file.read((char *)&referenceAverage, sizeof(double))) {
    throw std::runtime_error("Failed to read reference average from binary file");
  }

  return referenceAverage;
}

void HaversineProcessor::computeDistances() {
  TIME_BLOCK("ComputeDistances");
  {
    TIME_BLOCK("ClearDistances");
    distances.clear();
    sum = 0.0;
  }

  {
    TIME_BLOCK("LoopAndSum");
    for (const auto &pair : pairs) {
      double distance = computeHaversine(pair.p0, pair.p1);
      distances.push_back(distance);
      sum += distance;
    }
  }
}

double HaversineProcessor::getAverage() const {
  if (distances.empty()) {
    return 0.0;
  }
  return sum / distances.size();
}

void HaversineProcessor::compareWithReference(double referenceSum) {
  TIME_BLOCK("CompareWithReference");
  {
    TIME_BLOCK("ComputeStats");
    double computedAverage = getAverage();
    double diff = std::abs(computedAverage - referenceSum);
    double relativeError = diff / referenceSum;

    {
      TIME_BLOCK("PrintOutput");
      std::cout << "Reference average: " << referenceSum << "\n";
      std::cout << "Computed average:  " << computedAverage << "\n";
      std::cout << "Difference:        " << diff << "\n";
      std::cout << "Relative error:    " << (relativeError * 100.0) << "%\n";
      std::cout << "Sum:               " << sum << "\n";
    }
  }
}
