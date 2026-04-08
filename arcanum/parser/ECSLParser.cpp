/// \file
/// ECSL contract comment parser implementation.
/// This file contains the lexer; the recursive-descent parser and
/// pretty-printer are added in subsequent commits.

#include "parser/ECSLParser.h"

#include <cctype>
#include <cstddef>
#include <cstdint>
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

} // namespace

//===----------------------------------------------------------------------===//
// Public API
//===----------------------------------------------------------------------===//

ParseResult parseContract(std::string_view input) {
  ParseResult result;
  Lexer lexer(input);
  lexer.tokenize(result.errors);
  return result;
}

std::string toString(const Expr& /*expr*/) { return {}; }

std::string toString(const Clause& /*clause*/) { return {}; }

} // namespace arcanum::parser
