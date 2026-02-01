#include "haversine/parser.hpp"
#include "common/profiler.hpp"

#include <cctype>
#include <istream>

Parser::Parser(std::istream &is) : in(is) {}

[[noreturn]] void Parser::error(const std::string &m) const {
  std::string msg = "Parse error at line " + std::to_string(line) + ", col " +
                    std::to_string(col) + ": " + m;
  throw ParseError(msg, line, col);
}

ParseError::ParseError(const std::string &msg, size_t l, size_t c)
    : std::runtime_error(msg), line(l), col(c) {}

void Parser::bump(int c) {
  if (c == '\n') {
    ++line;
    col = 1;
  } else {
    ++col;
  }
}

int Parser::peek() { return in.peek(); }

int Parser::get() {
  int c = in.get();
  if (c == EOF)
    error("Unexpected end of input");
  bump(c);
  return c;
}

void Parser::skip_ws() {
  while (true) {
    int c = peek();
    if (c == EOF || !std::isspace((unsigned char)c))
      break;
    get();
  }
}

void Parser::expect(char ch) {
  skip_ws();
  if (get() != ch) {
    std::string m = "Expected '";
    m += ch;
    m += "'";
    error(m);
  }
}

bool Parser::match(char ch) {
  skip_ws();
  if (peek() == ch) {
    get();
    return true;
  }
  return false;
}

std::string Parser::parse_string() {
  skip_ws();
  if (get() != '"')
    error("Expected '\"'");

  std::string out;
  while (true) {
    int c = get();
    if (c == '"')
      break;

    out.push_back((char)c);
  }

  return out;
}

double Parser::parse_number() {
  skip_ws();

  std::string buf;
  int c = peek();
  if (!(c == '-' || (c >= '0' && c <= '9')))
    error("Expected number");

  while (true) {
    c = peek();
    if (c == '-' || c == '+' || c == '.' || c == 'e' || c == 'E' ||
        (c >= '0' && c <= '9')) {
      buf.push_back((char)get());
    } else
      break;
  }

  char *end = nullptr;
  double v = std::strtod(buf.c_str(), &end);
  if (!end || *end != '\0')
    error("Invalid number");
  return v;
}

std::vector<PointPair> Parser::parse_pairs_array() {
  expect('[');
  std::vector<PointPair> out;

  skip_ws();
  if (match(']'))
    return out;

  out.push_back(parse_pair_object());
  while (match(','))
    out.push_back(parse_pair_object());

  expect(']');
  return out;
}

PointPair Parser::parse_pair_object() {
  expect('{');
  PointPair p{};
  int seen = 0; // x0=1, y0=2, x1=4, y1=8

  skip_ws();
  if (match('}'))
    error("Empty pair object");

  while (true) {
    auto k = parse_string();
    expect(':');
    double v = parse_number();

    if (k == "x0") {
      p.p0.x = v;
      seen |= 1;
    } else if (k == "y0") {
      p.p0.y = v;
      seen |= 2;
    } else if (k == "x1") {
      p.p1.x = v;
      seen |= 4;
    } else if (k == "y1") {
      p.p1.y = v;
      seen |= 8;
    } else
      error("Unexpected key: " + k);

    if (match(','))
      continue;
    break;
  }

  expect('}');
  if (seen != 0b1111)
    error("Missing x0/y0/x1/y1");
  return p;
}

std::vector<PointPair> Parser::parse_document() {
  skip_ws();
  expect('{');

  auto key = parse_string();
  if (key != "pairs") {
    error("Wrong key encountered. Expected \"pairs\".");
  }

  expect(':');
  auto pairs = parse_pairs_array();

  skip_ws();
  expect('}');

  if (in.peek() != EOF) {
    error("Trailing characters after JSON");
  }

  return pairs;
}
