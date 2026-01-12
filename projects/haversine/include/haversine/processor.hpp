#pragma once

#include <cstdint>
#include <stdexcept>
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

struct ParseError : std::runtime_error {
  size_t line, col;
  ParseError(const std::string &msg, size_t l, size_t c);
};

class Parser {
public:
  explicit Parser(std::istream &is);

  // Parses:
  // {"pairs":[ {"x0":..., "y0":..., "x1":..., "y1":...}, ... ]}
  std::vector<PointPair> parse_document();

private:
  std::istream &in;
  size_t line = 1, col = 1;

  [[noreturn]] void error(const std::string &m) const;
  void bump(int c);

  int peek();
  int get();
  void skip_ws();
  void expect(char ch);
  bool match(char ch);

  std::string parse_string();
  double parse_number();

  std::vector<PointPair> parse_pairs_array();
  PointPair parse_pair_object();
};

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
