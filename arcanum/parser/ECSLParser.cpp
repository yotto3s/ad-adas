/// \file
/// ECSL contract comment parser implementation.
/// Contains the lexer, recursive-descent parser, and pretty-printer.

#include "parser/ECSLParser.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace arcanum::parser {

namespace {

//===----------------------------------------------------------------------===//
// Tokens
//===----------------------------------------------------------------------===//

enum class TokKind : std::uint8_t {
  // Keywords
  KwRequires,
  KwEnsures,
  KwAssigns,
  KwResult,
  KwNothing,
  // Atoms
  Ident,
  IntLit,
  // Punctuation
  LParen,
  RParen,
  Semi,
  // Operators
  EqEq,
  BangEq,
  Lt,
  Le,
  Gt,
  Ge,
  Plus,
  Minus,
  Star,
  Slash,
  Percent,
  Bang,
  AmpAmp,
  PipePipe,
  // Control
  Eof,
  Error,
};

struct Token {
  TokKind kind = TokKind::Eof;
  std::string text;
  SourceLoc loc;
};

class Lexer {
public:
  explicit Lexer(std::string_view src) : input(src) {}

  std::vector<Token> tokenize(std::vector<ParseError>& errors) {
    std::vector<Token> out;
    while (true) {
      skipWhitespace();
      Token tok = lexOne();
      if (tok.kind == TokKind::Error) {
        errors.push_back({std::move(tok.text), tok.loc});
        continue;
      }
      const bool done = tok.kind == TokKind::Eof;
      out.push_back(std::move(tok));
      if (done) {
        break;
      }
    }
    return out;
  }

private:
  Token lexOne() {
    if (atEnd()) {
      return makeAt(TokKind::Eof, "", loc);
    }
    const SourceLoc start = loc;
    const char c = peek();

    if ((std::isalpha(static_cast<unsigned char>(c)) != 0) || c == '_') {
      return lexIdentifier(start);
    }
    if (std::isdigit(static_cast<unsigned char>(c)) != 0) {
      return lexInteger(start);
    }
    if (c == '\\') {
      return lexBackslashKeyword(start);
    }
    return lexOperatorOrPunct(start);
  }

  Token lexIdentifier(SourceLoc start) {
    std::string text;
    while (!atEnd()) {
      const char c = peek();
      const bool isWord =
          (std::isalnum(static_cast<unsigned char>(c)) != 0) || c == '_';
      if (!isWord) {
        break;
      }
      text.push_back(c);
      advance();
    }
    if (text == "requires") {
      return makeAt(TokKind::KwRequires, std::move(text), start);
    }
    if (text == "ensures") {
      return makeAt(TokKind::KwEnsures, std::move(text), start);
    }
    if (text == "assigns") {
      return makeAt(TokKind::KwAssigns, std::move(text), start);
    }
    return makeAt(TokKind::Ident, std::move(text), start);
  }

  Token lexInteger(SourceLoc start) {
    std::string text;
    while (!atEnd() &&
           (std::isdigit(static_cast<unsigned char>(peek())) != 0)) {
      text.push_back(peek());
      advance();
    }
    return makeAt(TokKind::IntLit, std::move(text), start);
  }

  Token lexBackslashKeyword(SourceLoc start) {
    advance(); // consume '\'
    std::string text;
    while (!atEnd()) {
      const char c = peek();
      const bool isWord =
          (std::isalpha(static_cast<unsigned char>(c)) != 0) || c == '_';
      if (!isWord) {
        break;
      }
      text.push_back(c);
      advance();
    }
    if (text == "result") {
      return makeAt(TokKind::KwResult, "\\result", start);
    }
    if (text == "nothing") {
      return makeAt(TokKind::KwNothing, "\\nothing", start);
    }
    return makeAt(TokKind::Error, "unknown backslash keyword: \\" + text,
                  start);
  }

  Token lexOperatorOrPunct(SourceLoc start) {
    const char c = peek();
    advance();
    switch (c) {
    case '(':
      return makeAt(TokKind::LParen, "(", start);
    case ')':
      return makeAt(TokKind::RParen, ")", start);
    case ';':
      return makeAt(TokKind::Semi, ";", start);
    case '+':
      return makeAt(TokKind::Plus, "+", start);
    case '-':
      return makeAt(TokKind::Minus, "-", start);
    case '*':
      return makeAt(TokKind::Star, "*", start);
    case '/':
      return makeAt(TokKind::Slash, "/", start);
    case '%':
      return makeAt(TokKind::Percent, "%", start);
    case '=':
      if (!atEnd() && peek() == '=') {
        advance();
        return makeAt(TokKind::EqEq, "==", start);
      }
      return makeAt(TokKind::Error, "unexpected '='", start);
    case '!':
      if (!atEnd() && peek() == '=') {
        advance();
        return makeAt(TokKind::BangEq, "!=", start);
      }
      return makeAt(TokKind::Bang, "!", start);
    case '<':
      if (!atEnd() && peek() == '=') {
        advance();
        return makeAt(TokKind::Le, "<=", start);
      }
      return makeAt(TokKind::Lt, "<", start);
    case '>':
      if (!atEnd() && peek() == '=') {
        advance();
        return makeAt(TokKind::Ge, ">=", start);
      }
      return makeAt(TokKind::Gt, ">", start);
    case '&':
      if (!atEnd() && peek() == '&') {
        advance();
        return makeAt(TokKind::AmpAmp, "&&", start);
      }
      return makeAt(TokKind::Error, "unexpected '&'", start);
    case '|':
      if (!atEnd() && peek() == '|') {
        advance();
        return makeAt(TokKind::PipePipe, "||", start);
      }
      return makeAt(TokKind::Error, "unexpected '|'", start);
    default:
      return makeAt(TokKind::Error,
                    std::string("unexpected character: '") + c + "'", start);
    }
  }

  void skipWhitespace() {
    while (!atEnd()) {
      const char c = peek();
      if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
        break;
      }
      advance();
    }
  }

  [[nodiscard]] bool atEnd() const { return pos >= input.size(); }
  [[nodiscard]] char peek() const { return input[pos]; }

  void advance() {
    if (atEnd()) {
      return;
    }
    if (input[pos] == '\n') {
      ++loc.line;
      loc.col = 1;
    } else {
      ++loc.col;
    }
    ++pos;
  }

  static Token makeAt(TokKind kind, std::string text, SourceLoc loc) {
    Token tok;
    tok.kind = kind;
    tok.text = std::move(text);
    tok.loc = loc;
    return tok;
  }

  std::string_view input;
  std::size_t pos = 0;
  SourceLoc loc;
};

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

// Recursive-descent parser: mutually recursive by design.
// NOLINTBEGIN(misc-no-recursion)
class Parser {
public:
  Parser(std::vector<Token> toks, std::vector<ParseError>& errs)
      : tokens(std::move(toks)), errors(errs) {}

  std::vector<Clause> parseClauses() {
    std::vector<Clause> clauses;
    unsigned index = 0;
    while (current().kind != TokKind::Eof) {
      if (auto clause = parseClause(index)) {
        clauses.push_back(std::move(*clause));
        ++index;
      } else {
        syncToNextClause();
      }
    }
    return clauses;
  }

private:
  std::optional<Clause> parseClause(unsigned index) {
    const Token startTok = current();
    Clause clause;
    clause.loc = startTok.loc;
    clause.index = index;

    if (startTok.kind == TokKind::KwRequires) {
      clause.kind = ClauseKind::Requires;
      advance();
      clause.predicate = parsePredicate();
    } else if (startTok.kind == TokKind::KwEnsures) {
      clause.kind = ClauseKind::Ensures;
      advance();
      clause.predicate = parsePredicate();
    } else if (startTok.kind == TokKind::KwAssigns) {
      clause.kind = ClauseKind::AssignsNothing;
      advance();
      if (!expect(TokKind::KwNothing, "expected '\\nothing' after 'assigns'")) {
        return std::nullopt;
      }
    } else {
      reportAt("expected 'requires', 'ensures', or 'assigns'", startTok.loc);
      return std::nullopt;
    }

    if (!expect(TokKind::Semi, "expected ';' at end of clause")) {
      return std::nullopt;
    }
    return clause;
  }

  // predicate := logical_or
  ExprPtr parsePredicate() { return parseLogicalOr(); }

  ExprPtr parseLogicalOr() {
    ExprPtr lhs = parseLogicalAnd();
    while (current().kind == TokKind::PipePipe) {
      const SourceLoc loc = current().loc;
      advance();
      ExprPtr rhs = parseLogicalAnd();
      lhs =
          makeBinary(BinaryOp::LogicalOr, std::move(lhs), std::move(rhs), loc);
    }
    return lhs;
  }

  ExprPtr parseLogicalAnd() {
    ExprPtr lhs = parseEquality();
    while (current().kind == TokKind::AmpAmp) {
      const SourceLoc loc = current().loc;
      advance();
      ExprPtr rhs = parseEquality();
      lhs =
          makeBinary(BinaryOp::LogicalAnd, std::move(lhs), std::move(rhs), loc);
    }
    return lhs;
  }

  ExprPtr parseEquality() {
    ExprPtr lhs = parseRelational();
    while (current().kind == TokKind::EqEq ||
           current().kind == TokKind::BangEq) {
      const BinaryOp op =
          current().kind == TokKind::EqEq ? BinaryOp::Eq : BinaryOp::Ne;
      const SourceLoc loc = current().loc;
      advance();
      ExprPtr rhs = parseRelational();
      lhs = makeBinary(op, std::move(lhs), std::move(rhs), loc);
    }
    return lhs;
  }

  ExprPtr parseRelational() {
    ExprPtr lhs = parseAdditive();
    while (true) {
      const TokKind k = current().kind;
      BinaryOp op = BinaryOp::Eq;
      if (k == TokKind::Lt) {
        op = BinaryOp::Lt;
      } else if (k == TokKind::Le) {
        op = BinaryOp::Le;
      } else if (k == TokKind::Gt) {
        op = BinaryOp::Gt;
      } else if (k == TokKind::Ge) {
        op = BinaryOp::Ge;
      } else {
        return lhs;
      }
      const SourceLoc loc = current().loc;
      advance();
      ExprPtr rhs = parseAdditive();
      lhs = makeBinary(op, std::move(lhs), std::move(rhs), loc);
    }
  }

  ExprPtr parseAdditive() {
    ExprPtr lhs = parseMultiplicative();
    while (current().kind == TokKind::Plus ||
           current().kind == TokKind::Minus) {
      const BinaryOp op =
          current().kind == TokKind::Plus ? BinaryOp::Add : BinaryOp::Sub;
      const SourceLoc loc = current().loc;
      advance();
      ExprPtr rhs = parseMultiplicative();
      lhs = makeBinary(op, std::move(lhs), std::move(rhs), loc);
    }
    return lhs;
  }

  ExprPtr parseMultiplicative() {
    ExprPtr lhs = parseUnary();
    while (true) {
      const TokKind k = current().kind;
      BinaryOp op = BinaryOp::Mul;
      if (k == TokKind::Star) {
        op = BinaryOp::Mul;
      } else if (k == TokKind::Slash) {
        op = BinaryOp::Div;
      } else if (k == TokKind::Percent) {
        op = BinaryOp::Mod;
      } else {
        return lhs;
      }
      const SourceLoc loc = current().loc;
      advance();
      ExprPtr rhs = parseUnary();
      lhs = makeBinary(op, std::move(lhs), std::move(rhs), loc);
    }
  }

  ExprPtr parseUnary() {
    if (current().kind == TokKind::Bang) {
      const SourceLoc loc = current().loc;
      advance();
      return makeUnary(UnaryOp::Not, parseUnary(), loc);
    }
    if (current().kind == TokKind::Minus) {
      const SourceLoc loc = current().loc;
      advance();
      return makeUnary(UnaryOp::Neg, parseUnary(), loc);
    }
    return parsePrimary();
  }

  ExprPtr parsePrimary() {
    const Token tok = current();
    if (tok.kind == TokKind::Ident) {
      advance();
      return makeExpr(Identifier{tok.text}, tok.loc);
    }
    if (tok.kind == TokKind::IntLit) {
      advance();
      return makeExpr(IntLiteral{tok.text}, tok.loc);
    }
    if (tok.kind == TokKind::KwResult) {
      advance();
      return makeExpr(ResultExpr{}, tok.loc);
    }
    if (tok.kind == TokKind::LParen) {
      advance();
      ExprPtr inner = parsePredicate();
      expect(TokKind::RParen, "expected ')' to close expression");
      return inner;
    }
    reportAt("expected expression, got '" + tok.text + "'", tok.loc);
    advance();
    return makeExpr(IntLiteral{"0"}, tok.loc); // error recovery
  }

  bool expect(TokKind kind, const std::string& message) {
    if (current().kind == kind) {
      advance();
      return true;
    }
    reportAt(message, current().loc);
    return false;
  }

  void syncToNextClause() {
    while (current().kind != TokKind::Eof) {
      if (current().kind == TokKind::Semi) {
        advance();
        return;
      }
      advance();
    }
  }

  void reportAt(std::string message, SourceLoc loc) {
    errors.push_back({std::move(message), loc});
  }

  [[nodiscard]] const Token& current() const { return tokens[cursor]; }

  void advance() {
    if (tokens[cursor].kind != TokKind::Eof) {
      ++cursor;
    }
  }

  static ExprPtr makeBinary(BinaryOp op, ExprPtr lhs, ExprPtr rhs,
                            SourceLoc loc) {
    auto expr = std::make_unique<Expr>();
    expr->node = BinaryExpr{op, std::move(lhs), std::move(rhs)};
    expr->loc = loc;
    return expr;
  }

  static ExprPtr makeUnary(UnaryOp op, ExprPtr operand, SourceLoc loc) {
    auto expr = std::make_unique<Expr>();
    expr->node = UnaryExpr{op, std::move(operand)};
    expr->loc = loc;
    return expr;
  }

  template <typename T> static ExprPtr makeExpr(T node, SourceLoc loc) {
    auto expr = std::make_unique<Expr>();
    expr->node = std::move(node);
    expr->loc = loc;
    return expr;
  }

  std::vector<Token> tokens;
  std::vector<ParseError>& errors;
  std::size_t cursor = 0;
};
// NOLINTEND(misc-no-recursion)

//===----------------------------------------------------------------------===//
// Pretty-printing
//===----------------------------------------------------------------------===//

const char* binaryOpSpelling(BinaryOp op) {
  switch (op) {
  case BinaryOp::Eq:
    return "==";
  case BinaryOp::Ne:
    return "!=";
  case BinaryOp::Lt:
    return "<";
  case BinaryOp::Le:
    return "<=";
  case BinaryOp::Gt:
    return ">";
  case BinaryOp::Ge:
    return ">=";
  case BinaryOp::Add:
    return "+";
  case BinaryOp::Sub:
    return "-";
  case BinaryOp::Mul:
    return "*";
  case BinaryOp::Div:
    return "/";
  case BinaryOp::Mod:
    return "%";
  case BinaryOp::LogicalAnd:
    return "&&";
  case BinaryOp::LogicalOr:
    return "||";
  }
  return "?";
}

const char* unaryOpSpelling(UnaryOp op) {
  switch (op) {
  case UnaryOp::Neg:
    return "-";
  case UnaryOp::Not:
    return "!";
  }
  return "?";
}

// Expression printing recurses through nested Binary / Unary nodes.
// NOLINTBEGIN(misc-no-recursion)
void printExpr(std::ostringstream& out, const Expr& expr) {
  std::visit(
      [&out](const auto& node) {
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, Identifier>) {
          out << node.name;
        } else if constexpr (std::is_same_v<T, IntLiteral>) {
          out << node.text;
        } else if constexpr (std::is_same_v<T, ResultExpr>) {
          out << "\\result";
        } else if constexpr (std::is_same_v<T, BinaryExpr>) {
          out << "(" << binaryOpSpelling(node.op) << " ";
          printExpr(out, *node.lhs);
          out << " ";
          printExpr(out, *node.rhs);
          out << ")";
        } else if constexpr (std::is_same_v<T, UnaryExpr>) {
          out << "(" << unaryOpSpelling(node.op) << " ";
          printExpr(out, *node.operand);
          out << ")";
        }
      },
      expr.node);
}
// NOLINTEND(misc-no-recursion)

const char* clauseKindSpelling(ClauseKind kind) {
  switch (kind) {
  case ClauseKind::Requires:
    return "requires";
  case ClauseKind::Ensures:
    return "ensures";
  case ClauseKind::AssignsNothing:
    return "assigns \\nothing";
  }
  return "?";
}

} // namespace

//===----------------------------------------------------------------------===//
// Public API
//===----------------------------------------------------------------------===//

ParseResult parseContract(std::string_view input) {
  ParseResult result;
  Lexer lexer(input);
  auto tokens = lexer.tokenize(result.errors);
  Parser parser(std::move(tokens), result.errors);
  result.clauses = parser.parseClauses();
  return result;
}

std::string toString(const Expr& expr) {
  std::ostringstream out;
  printExpr(out, expr);
  return out.str();
}

std::string toString(const Clause& clause) {
  std::ostringstream out;
  out << "[" << clause.index << "] " << clauseKindSpelling(clause.kind);
  if (clause.predicate) {
    out << " ";
    printExpr(out, *clause.predicate);
  }
  return out.str();
}

} // namespace arcanum::parser
