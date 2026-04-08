// Unit test for arcanum::parser::parseContract: parses a handful of
// representative ECSL contract bodies and checks the pretty-printed
// AST against expected strings.

#include "parser/ECSLParser.h"

#include <array>
#include <iostream>
#include <string>
#include <string_view>

namespace {

struct Case {
  std::string_view name;
  std::string_view input;
  std::string_view expected;
};

bool run(const Case &c) {
  const arcanum::parser::ParseResult result =
      arcanum::parser::parseContract(c.input);
  std::string actual;
  for (const auto &err : result.errors) {
    actual += "error@" + std::to_string(err.loc.line) + ":" +
              std::to_string(err.loc.col) + ": " + err.message + "\n";
  }
  for (const auto &clause : result.clauses) {
    actual += toString(clause);
    actual += "\n";
  }
  if (actual != std::string(c.expected)) {
    std::cerr << "FAIL: " << c.name << "\n"
              << "  input:    " << c.input << "\n"
              << "  expected: |" << c.expected << "|\n"
              << "  actual:   |" << actual << "|\n";
    return false;
  }
  return true;
}

} // namespace

int main() {
  const std::array<Case, 9> cases{{
      {"single_requires", "requires x >= 0;", "[0] requires (>= x 0)\n"},
      {"single_ensures_with_result", "ensures \\result >= 0;",
       "[0] ensures (>= \\result 0)\n"},
      {"assigns_nothing", "assigns \\nothing;", "[0] assigns \\nothing\n"},
      {"multiple_clauses",
       "requires x > 0; requires y > 0; ensures \\result == x + y;",
       "[0] requires (> x 0)\n"
       "[1] requires (> y 0)\n"
       "[2] ensures (== \\result (+ x y))\n"},
      {"precedence", "requires a + b * c <= d;",
       "[0] requires (<= (+ a (* b c)) d)\n"},
      {"logical_connectives", "requires x > 0 && (y > 0 || z == 0);",
       "[0] requires (&& (> x 0) (|| (> y 0) (== z 0)))\n"},
      {"unary_not_and_neg", "requires !(x == 0) && y == -1;",
       "[0] requires (&& (! (== x 0)) (== y (- 1)))\n"},
      {"error_missing_semicolon", "requires x > 0",
       "error@1:15: expected ';' at end of clause\n"},
      {"error_bad_token", "requires x & y;",
       "error@1:12: unexpected '&'\n"
       "error@1:14: expected ';' at end of clause\n"},
  }};

  bool allPassed = true;
  for (const auto &c : cases) {
    if (!run(c)) {
      allPassed = false;
    }
  }
  return allPassed ? 0 : 1;
}
