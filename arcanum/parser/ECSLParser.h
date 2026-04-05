/// \file
/// ECSL contract comment parser: tokenizes and parses the body of a
/// `/*@ requires ... ensures ... */` annotation into a structured
/// clause tree.

#ifndef PARSER_ECSLPARSER_H
#define PARSER_ECSLPARSER_H

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace arcanum::parser {

/// 1-based line and column in the contract comment body.
struct SourceLoc {
  unsigned line = 1; ///< 1-based line number.
  unsigned col = 1;  ///< 1-based column number.
};

/// Binary operators recognized by the ECSL contract parser.
enum class BinaryOp : std::uint8_t {
  // Comparison
  Eq,
  Ne,
  Lt,
  Le,
  Gt,
  Ge,
  // Arithmetic
  Add,
  Sub,
  Mul,
  Div,
  Mod,
  // Logical
  LogicalAnd,
  LogicalOr,
};

/// Unary operators recognized by the ECSL contract parser.
enum class UnaryOp : std::uint8_t {
  Neg, ///< arithmetic negation `-x`
  Not, ///< logical negation `!p`
};

struct Expr;
/// Owning handle to an expression node.
using ExprPtr = std::unique_ptr<Expr>;

/// A bare identifier expression.
struct Identifier {
  std::string name; ///< Identifier spelling.
};

/// A decimal integer literal.
struct IntLiteral {
  std::int64_t value; ///< Decoded literal value.
};

/// The special `\\result` identifier referring to an ensures-clause
/// function return value.
struct ResultExpr {};

/// A binary expression node.
struct BinaryExpr {
  BinaryOp op; ///< Operator.
  ExprPtr lhs; ///< Left operand.
  ExprPtr rhs; ///< Right operand.
};

/// A unary expression node.
struct UnaryExpr {
  UnaryOp op;      ///< Operator.
  ExprPtr operand; ///< Single operand.
};

/// An expression tree node with source location.
struct Expr {
  /// Concrete node payload.
  std::variant<Identifier, IntLiteral, ResultExpr, BinaryExpr, UnaryExpr> node;
  SourceLoc loc; ///< Source location of the expression's head token.
};

/// Kind of a parsed top-level contract clause.
enum class ClauseKind : std::uint8_t {
  Requires,
  Ensures,
  AssignsNothing,
};

/// One parsed contract clause.
struct Clause {
  ClauseKind kind; ///< Kind of this clause.
  /// `nullptr` for `AssignsNothing`, otherwise the parsed predicate.
  ExprPtr predicate;
  SourceLoc loc;      ///< Source location of the clause keyword.
  unsigned index = 0; ///< 0-based index within the parsed comment,
                      ///< used by the caller to synthesize
                      ///< `ecsl.property_id` attributes.
};

/// A single parser or lexer diagnostic.
struct ParseError {
  std::string message; ///< Human-readable message.
  SourceLoc loc;       ///< Source location where the error was raised.
};

/// Result of parsing a contract body: parsed clauses plus any errors
/// encountered during lexing/parsing. Partial progress is preserved
/// in `clauses` even when `errors` is non-empty.
struct ParseResult {
  std::vector<Clause> clauses;    ///< Successfully parsed clauses.
  std::vector<ParseError> errors; ///< Lex/parse diagnostics.

  /// Returns `true` if parsing produced no errors.
  [[nodiscard]] bool ok() const { return errors.empty(); }
};

/// Parse the contents of an ECSL contract comment body (i.e. the text
/// between `/*@` and `*/`, without those delimiters). The body may
/// contain one or more clauses separated by `;`.
ParseResult parseContract(std::string_view input);

/// Pretty-print an expression as an S-expression for tests /
/// diagnostics.
std::string toString(const Expr &expr);

/// Pretty-print a clause.
std::string toString(const Clause &clause);

} // namespace arcanum::parser

#endif // PARSER_ECSLPARSER_H
