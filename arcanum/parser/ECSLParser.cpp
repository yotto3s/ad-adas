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
      return makeToken(TokKind::Eof, "", loc);
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
      return makeToken(TokKind::KwRequires, std::move(text), start);
    }
    if (text == "ensures") {
      return makeToken(TokKind::KwEnsures, std::move(text), start);
    }
    if (text == "assigns") {
      return makeToken(TokKind::KwAssigns, std::move(text), start);
    }
    return makeToken(TokKind::Ident, std::move(text), start);
  }

  Token lexInteger(SourceLoc start) {
    std::string text;
    while (!atEnd() &&
           (std::isdigit(static_cast<unsigned char>(peek())) != 0)) {
      text.push_back(peek());
      advance();
    }
    return makeToken(TokKind::IntLit, std::move(text), start);
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
      return makeToken(TokKind::KwResult, "\\result", start);
    }
    if (text == "nothing") {
      return makeToken(TokKind::KwNothing, "\\nothing", start);
    }
    return makeToken(TokKind::Error, "unknown backslash keyword: \\" + text,
                  start);
  }

  Token lexOperatorOrPunct(SourceLoc start) {
    const char c = peek();
    advance();
    switch (c) {
    case '(':
      return makeToken(TokKind::LParen, "(", start);
    case ')':
      return makeToken(TokKind::RParen, ")", start);
    case ';':
      return makeToken(TokKind::Semi, ";", start);
    case '+':
      return makeToken(TokKind::Plus, "+", start);
    case '-':
      return makeToken(TokKind::Minus, "-", start);
    case '*':
      return makeToken(TokKind::Star, "*", start);
    case '/':
      return makeToken(TokKind::Slash, "/", start);
    case '%':
      return makeToken(TokKind::Percent, "%", start);
    case '=':
      if (!atEnd() && peek() == '=') {
        advance();
        return makeToken(TokKind::EqEq, "==", start);
      }
      return makeToken(TokKind::Error, "unexpected '='", start);
    case '!':
      if (!atEnd() && peek() == '=') {
        advance();
        return makeToken(TokKind::BangEq, "!=", start);
      }
      return makeToken(TokKind::Bang, "!", start);
    case '<':
      if (!atEnd() && peek() == '=') {
        advance();
        return makeToken(TokKind::Le, "<=", start);
      }
      return makeToken(TokKind::Lt, "<", start);
    case '>':
      if (!atEnd() && peek() == '=') {
        advance();
        return makeToken(TokKind::Ge, ">=", start);
      }
      return makeToken(TokKind::Gt, ">", start);
    case '&':
      if (!atEnd() && peek() == '&') {
        advance();
        return makeToken(TokKind::AmpAmp, "&&", start);
      }
      return makeToken(TokKind::Error, "unexpected '&'", start);
    case '|':
      if (!atEnd() && peek() == '|') {
        advance();
        return makeToken(TokKind::PipePipe, "||", start);
      }
      return makeToken(TokKind::Error, "unexpected '|'", start);
    default:
      return makeToken(TokKind::Error,
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

  [[nodiscard]] static Token makeToken(TokKind kind, std::string text, SourceLoc loc) {
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
  const auto tokens = lexer.tokenize(result.errors);
  std::ignore = tokens;
  //TODO: implement parsing here
  return result;
}

std::string toString(const Expr& /*expr*/) { return {}; }

std::string toString(const Clause& /*clause*/) { return {}; }

} // namespace arcanum::parser
