#pragma once

#include "haversine/parser.hpp"

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

class HaversineProcessor {
public:
  // Parse JSON file and extract point pairs
  void parseJSON(const std::string &filename);

  // Read reference data from binary file
  double readBinaryReference(const std::string &filename);

  // Calculate haversine distances for all pairs
  void computeDistances();

  // Get results
  const std::vector<double> &getDistances() const { return distances; }
  double getSum() const { return sum; }
  double getAverage() const;

  // Compare with reference sum
  void compareWithReference(double referenceSum);

private:
  std::vector<PointPair> pairs;
  std::vector<double> distances;
  double sum = 0.0;

  // Helper: compute haversine distance
  static double computeHaversine(const Point &p0, const Point &p1);

  // Helper: convert degrees to radians
  static double degreesToRadians(double degrees);
};
