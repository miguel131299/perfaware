#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct Point {
  double x;
  double y;
};

struct PointPair {
  Point p0;
  Point p1;
};

class HaversineGenerator {
public:
  // Generate N random point pairs with optional clustering
  // gridSize: number of cells per dimension (e.g., 2 = 2x2 grid = 4 clusters, 3 = 3x3 grid = 9 clusters)
  explicit HaversineGenerator(uint64_t pairCount, uint64_t seed = 0, bool useClustering = true, int gridSize = 2);
  
  // Get the generated pairs
  const std::vector<PointPair>& getPairs() const { return pairs; }
  
  // Get the computed reference sum of all haversine distances
  double getExpectedSum() const { return expectedSum; }
  
  // Get the seed used
  uint64_t getSeed() const { return seed; }
  
  // Get the grid size used
  int getGridSize() const { return gridSize; }
  
  // Output as JSON
  std::string toJSON() const;
  
  // Write binary .f64 file with distances and total sum
  void writeBinaryResults(const std::string& filename) const;
  
private:
  std::vector<PointPair> pairs;
  std::vector<double> distances;
  double expectedSum;
  uint64_t seed;
  int gridSize;
  
  // Compute haversine distance between two points (in degrees)
  static double computeHaversine(const Point& p0, const Point& p1);
  
  // Generate random value in range [0, 1)
  double randomDouble();
  
  // Generate random value in range [min, max)
  double randomDouble(double min, double max);
};
