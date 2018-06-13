// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/RegExprParser.h>
#include <klex/RegExpr.h>

#include <sstream>
#include <limits>
#include <iostream>

#include <fmt/format.h>

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

/*
  REGULAR EXPRESSION SYNTAX:
  --------------------------

  expr                    := alternation
  alternation             := concatenation ('|' concatenation)*
  concatenation           := closure (closure)*
  closure                 := atom ['*' | '?' | '{' NUM [',' NUM] '}']
  atom                    := character | characterClass | '(' expr ')'
  characterClass          := '[' ['^'] characterClassFragment+ ']'
  characterClassFragment  := character | character '-' character
*/

namespace klex {

RegExprParser::RegExprParser()
    : input_{},
      currentChar_{input_.end()} {
}

int RegExprParser::currentChar() const {
  if (currentChar_ != input_.end())
    return *currentChar_;
  else
    return -1;
}

bool RegExprParser::consumeIf(int ch) {
  if (currentChar() != ch)
    return false;

  consume();
  return true;
}

int RegExprParser::consume() {
  if (currentChar_ == input_.end())
    return -1;

  int ch = *currentChar_;
  ++currentChar_;
  //DEBUG("consume: '{}'", (char)ch);
  return ch;
}

void RegExprParser::consume(int expected) {
  int actual = currentChar();
  consume();
  if (actual != expected) {
    throw UnexpectedToken{actual, expected};
  }
}

std::unique_ptr<RegExpr> RegExprParser::parse(std::string_view expr) {
  input_ = std::move(expr);
  currentChar_ = input_.begin();

  return parseExpr();
}

std::unique_ptr<RegExpr> RegExprParser::parseExpr() {
  return parseAlternation();
}

std::unique_ptr<RegExpr> RegExprParser::parseAlternation() {
  std::unique_ptr<RegExpr> lhs = parseConcatenation();

  while (currentChar() == '|') {
    consume();
    std::unique_ptr<RegExpr> rhs = parseConcatenation();
    lhs = std::make_unique<AlternationExpr>(std::move(lhs), std::move(rhs));
  }

  return lhs;
}

std::unique_ptr<RegExpr> RegExprParser::parseConcatenation() {
  // FOLLOW-set, the set of terminal tokens that can occur right after a concatenation
  static const std::string_view follow = "|)";
  std::unique_ptr<RegExpr> lhs = parseClosure();

  while (!eof() && follow.find(currentChar()) == follow.npos) {
    std::unique_ptr<RegExpr> rhs = parseClosure();
    lhs = std::make_unique<ConcatenationExpr>(std::move(lhs), std::move(rhs));
  }

  return lhs;
}

std::unique_ptr<RegExpr> RegExprParser::parseClosure() {
  std::unique_ptr<RegExpr> subExpr = parseAtom();

  switch (currentChar()) {
    case '?':
      consume();
      return std::make_unique<ClosureExpr>(std::move(subExpr), 0, 1);
    case '*':
      consume();
      return std::make_unique<ClosureExpr>(std::move(subExpr), 0);
    case '+':
      consume();
      return std::make_unique<ClosureExpr>(std::move(subExpr), 1);
    case '{': {
      consume();
      int m = parseInt();
      if (currentChar() == ',') {
        consume();
        int n = parseInt();
        consume('}');
        return std::make_unique<ClosureExpr>(std::move(subExpr), m, n);
      } else {
        consume('}');
        return std::make_unique<ClosureExpr>(std::move(subExpr), m, m);
      }
    }
    default:
      return subExpr;
  }
}

unsigned RegExprParser::parseInt() {
  unsigned n = 0;
  while (std::isdigit(currentChar())) {
    n *= 10;
    n += currentChar() - '0';
    consume();
  }
  return n;
}

std::unique_ptr<RegExpr> RegExprParser::parseAtom() {
  switch (currentChar()) {
    case '(': {
      consume();
      std::unique_ptr<RegExpr> subExpr = parseExpr();
      consume(')');
      return subExpr;
    }
    case '[':
      return parseCharacterClass();
    case '.':
      consume();
      return std::make_unique<DotExpr>();
    case '$':
      consume();
      return std::make_unique<EndOfLineExpr>();
    case '\\':
      consume();
      switch (currentChar()) {
        case 's':
          consume();
          return std::make_unique<CharacterExpr>(' ');
        case 't':
          consume();
          return std::make_unique<CharacterExpr>('\t');
        case 'n':
          consume();
          return std::make_unique<CharacterExpr>('\n');
        case 'r':
          consume();
          return std::make_unique<CharacterExpr>('\r');
        default:
          return std::make_unique<CharacterExpr>(consume());
      }
    default:
      return std::make_unique<CharacterExpr>(consume());
  }
}

std::unique_ptr<RegExpr> RegExprParser::parseCharacterClass() {
  consume(); // '['
  const bool complement = consumeIf('^');
  std::unique_ptr<RegExpr> e = parseCharacterClassFragment();
  while (currentChar() != ']')
    e = std::make_unique<AlternationExpr>(std::move(e), parseCharacterClassFragment());
  consume(']');
  return e;
}

std::unique_ptr<RegExpr> RegExprParser::parseCharacterClassFragment() {
  if (currentChar() == '\\') {
    consume();
    switch (currentChar()) {
      case 't':
        consume();
        return std::make_unique<CharacterExpr>('\t');
      case 'n':
        consume();
        return std::make_unique<CharacterExpr>('\n');
      case 'r':
        consume();
        return std::make_unique<CharacterExpr>('\r');
      default:
        return std::make_unique<CharacterExpr>(consume());
    }
  }

  char c1 = consume();
  if (currentChar() != '-')
    return std::make_unique<CharacterExpr>(c1);

  consume();
  char c2 = consume();

  std::unique_ptr<RegExpr> e = std::make_unique<CharacterExpr>(c1);
  for (char c_i = c1 + 1; c_i <= c2; c_i++)
    e = std::make_unique<AlternationExpr>(std::move(e),
            std::make_unique<CharacterExpr>(c_i));
  return e;
}

} // namespace klex
