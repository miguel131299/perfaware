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
