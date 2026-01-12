#include "haversine/haversine.hpp"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>

// Earth's radius in kilometers
static constexpr double EARTH_RADIUS_KM = 6371.0;

// Convert degrees to radians
static inline double degreesToRadians(double degrees) {
  return degrees * M_PI / 180.0;
}

static double square(double A) {
  double Result = (A * A);
  return Result;
}

double HaversineGenerator::computeHaversine(const Point& p0, const Point& p1) {
  double lat0 = degreesToRadians(p0.y);
  double lon0 = degreesToRadians(p0.x);
  double lat1 = degreesToRadians(p1.y);
  double lon1 = degreesToRadians(p1.x);
  
  double dLat = lat1 - lat0;
  double dLon = lon1 - lon0;
  
  double a = square(std::sin(dLat / 2.0)) +
             std::cos(lat0) * std::cos(lat1) * 
             square(std::sin(dLon / 2.0));
  
  double c = 2.0 * std::asin(std::sqrt(a));
  
  return EARTH_RADIUS_KM * c;
}

double HaversineGenerator::randomDouble() {
  // Linear congruential generator for reproducibility
  seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
  return static_cast<double>(seed >> 11) / static_cast<double>(1ULL << 53);
}

double HaversineGenerator::randomDouble(double min, double max) {
  return min + randomDouble() * (max - min);
}

HaversineGenerator::HaversineGenerator(uint64_t pairCount, uint64_t seedValue, bool useClustering, int gridSizeParam)
    : expectedSum(0.0), seed(seedValue), gridSize(gridSizeParam) {
  if (gridSize < 1) {
    throw std::invalid_argument("Grid size must be at least 1");
  }
  
  if (seed == 0) {
    std::random_device rd;
    seed = rd();
  }
  
  pairs.reserve(pairCount);
  distances.reserve(pairCount);
  
  if (useClustering) {
    // Use clustering: divide world into regions to avoid asymptotic averages
    // This generates more realistic distributions
    int clusterCount = gridSize * gridSize;
    double lonStep = 360.0 / gridSize;
    double latStep = 180.0 / gridSize;
    
    for (uint64_t i = 0; i < pairCount; ++i) {
      // Pick two random clusters
      int cluster0 = static_cast<int>(randomDouble() * clusterCount);
      int cluster1 = static_cast<int>(randomDouble() * clusterCount);
      
      // Clamp to valid range
      cluster0 = (cluster0 >= clusterCount) ? clusterCount - 1 : cluster0;
      cluster1 = (cluster1 >= clusterCount) ? clusterCount - 1 : cluster1;
      
      // Calculate grid indices
      int lon0_idx = cluster0 % gridSize;
      int lat0_idx = cluster0 / gridSize;
      int lon1_idx = cluster1 % gridSize;
      int lat1_idx = cluster1 / gridSize;
      
      // Define cluster bounds
      double lon0_min = -180.0 + lon0_idx * lonStep;
      double lon0_max = lon0_min + lonStep;
      double lat0_min = -90.0 + lat0_idx * latStep;
      double lat0_max = lat0_min + latStep;
      
      double lon1_min = -180.0 + lon1_idx * lonStep;
      double lon1_max = lon1_min + lonStep;
      double lat1_min = -90.0 + lat1_idx * latStep;
      double lat1_max = lat1_min + latStep;
      
      Point p0{randomDouble(lon0_min, lon0_max), randomDouble(lat0_min, lat0_max)};
      Point p1{randomDouble(lon1_min, lon1_max), randomDouble(lat1_min, lat1_max)};
      
      double distance = computeHaversine(p0, p1);
      
      pairs.push_back({p0, p1});
      distances.push_back(distance);
      expectedSum += distance;
    }
  } else {
    // Simple random generation without clustering
    for (uint64_t i = 0; i < pairCount; ++i) {
      Point p0{randomDouble(-180.0, 180.0), randomDouble(-90.0, 90.0)};
      Point p1{randomDouble(-180.0, 180.0), randomDouble(-90.0, 90.0)};
      
      double distance = computeHaversine(p0, p1);
      
      pairs.push_back({p0, p1});
      distances.push_back(distance);
      expectedSum += distance;
    }
  }
}

std::string HaversineGenerator::toJSON() const {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(15);
  
  oss << "{\"pairs\":[\n";
  
  for (size_t i = 0; i < pairs.size(); ++i) {
    const auto& pair = pairs[i];
    oss << "    {\"x0\":" << pair.p0.x << ", \"y0\":" << pair.p0.y
        << ", \"x1\":" << pair.p1.x << ", \"y1\":" << pair.p1.y << "}";
    
    if (i < pairs.size() - 1) {
      oss << ",";
    }
    oss << "\n";
  }
  
  oss << "]}\n";
  
  return oss.str();
}

void HaversineGenerator::writeBinaryResults(const std::string& filename) const {
  std::ofstream file(filename, std::ios::binary);
  
  if (!file) {
    throw std::runtime_error("Failed to open file for writing: " + filename);
  }
  
  // Write each distance as a double
  for (double distance : distances) {
    file.write(reinterpret_cast<const char*>(&distance), sizeof(double));
  }
  
  // Write the average as the final double
  double average = expectedSum / distances.size();
  file.write(reinterpret_cast<const char*>(&average), sizeof(double));
  
  file.close();
}
