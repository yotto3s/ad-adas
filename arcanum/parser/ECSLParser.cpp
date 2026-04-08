/// \file
/// ECSL contract comment parser implementation.
/// This file contains the lexer and recursive-descent parser; the
/// pretty-printer is added in a subsequent commit.

#include "parser/ECSLParser.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
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
      return Token{TokKind::Eof, "", loc};
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
      return Token{TokKind::KwRequires, std::move(text), start};
    }
    if (text == "ensures") {
      return Token{TokKind::KwEnsures, std::move(text), start};
    }
    if (text == "assigns") {
      return Token{TokKind::KwAssigns, std::move(text), start};
    }
    return Token{TokKind::Ident, std::move(text), start};
  }

  Token lexInteger(SourceLoc start) {
    std::string text;
    while (!atEnd() &&
           (std::isdigit(static_cast<unsigned char>(peek())) != 0)) {
      text.push_back(peek());
      advance();
    }
    return Token{TokKind::IntLit, std::move(text), start};
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
      return Token{TokKind::KwResult, "\\result", start};
    }
    if (text == "nothing") {
      return Token{TokKind::KwNothing, "\\nothing", start};
    }
    return Token{TokKind::Error, "unknown backslash keyword: \\" + text, start};
  }

  Token lexOperatorOrPunct(SourceLoc start) {
    const char c = peek();
    advance();
    switch (c) {
    case '(':
      return Token{TokKind::LParen, "(", start};
    case ')':
      return Token{TokKind::RParen, ")", start};
    case ';':
      return Token{TokKind::Semi, ";", start};
    case '+':
      return Token{TokKind::Plus, "+", start};
    case '-':
      return Token{TokKind::Minus, "-", start};
    case '*':
      return Token{TokKind::Star, "*", start};
    case '/':
      return Token{TokKind::Slash, "/", start};
    case '%':
      return Token{TokKind::Percent, "%", start};
    case '=':
      if (!atEnd() && peek() == '=') {
        advance();
        return Token{TokKind::EqEq, "==", start};
      }
      return Token{TokKind::Error, "unexpected '='", start};
    case '!':
      if (!atEnd() && peek() == '=') {
        advance();
        return Token{TokKind::BangEq, "!=", start};
      }
      return Token{TokKind::Bang, "!", start};
    case '<':
      if (!atEnd() && peek() == '=') {
        advance();
        return Token{TokKind::Le, "<=", start};
      }
      return Token{TokKind::Lt, "<", start};
    case '>':
      if (!atEnd() && peek() == '=') {
        advance();
        return Token{TokKind::Ge, ">=", start};
      }
      return Token{TokKind::Gt, ">", start};
    case '&':
      if (!atEnd() && peek() == '&') {
        advance();
        return Token{TokKind::AmpAmp, "&&", start};
      }
      return Token{TokKind::Error, "unexpected '&'", start};
    case '|':
      if (!atEnd() && peek() == '|') {
        advance();
        return Token{TokKind::PipePipe, "||", start};
      }
      return Token{TokKind::Error, "unexpected '|'", start};
    default:
      return Token{TokKind::Error,
                   std::string("unexpected character: '") + c + "'", start};
    }
  }

  void skipWhitespace() {
    while (!atEnd()) {
      const char c = peek();
      if (!isWhitespace(c)) {
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
    if (isNewline(input[pos])) {
      ++loc.line;
      loc.col = 1;
    } else {
      ++loc.col;
    }
    ++pos;
  }

  [[nodiscard]] static bool isWhitespace(const char c) noexcept {
    return c == ' ' || c == '\t' || isNewline(c);
  }

  [[nodiscard]] static bool isNewline(const char c) noexcept {
    return c == '\n' || c == '\r';
  }

  std::string_view input;
  std::size_t pos = 0;
  SourceLoc loc;
};

//===----------------------------------------------------------------------===//
// Parser
//===----------------------------------------------------------------------===//

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

std::string toString(const Expr& /*expr*/) { return {}; }

std::string toString(const Clause& /*clause*/) { return {}; }

} // namespace arcanum::parser
