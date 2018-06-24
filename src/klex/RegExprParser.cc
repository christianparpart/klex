// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/RegExprParser.h>
#include <klex/RegExpr.h>
#include <klex/Symbols.h>

#include <functional>
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
      currentChar_{input_.end()},
      line_{1},
      column_{0} {
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
  if (ch == '\n') {
    line_++;
    column_ = 1;
  } else {
    column_++;
  }
  ++currentChar_;
  DEBUG("consume: '{}'", (char)ch);
  return ch;
}

void RegExprParser::consume(int expected) {
  int actual = currentChar();
  consume();
  if (actual != expected) {
    throw UnexpectedToken{line_, column_, actual, expected};
  }
}

std::unique_ptr<RegExpr> RegExprParser::parse(std::string_view expr, int line, int column) {
  input_ = std::move(expr);
  currentChar_ = input_.begin();
  line_ = line;
  column_ = column;

  return parseExpr();
}

std::unique_ptr<RegExpr> RegExprParser::parseExpr() {
  return parseLookAheadExpr();
}

std::unique_ptr<RegExpr> RegExprParser::parseLookAheadExpr() {
  std::unique_ptr<RegExpr> lhs = parseAlternation();

  if (currentChar() == '/') {
    consume();
    std::unique_ptr<RegExpr> rhs = parseAlternation();
    lhs = std::make_unique<LookAheadExpr>(std::move(lhs), std::move(rhs));
  }

  return lhs;
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
  static const std::string_view follow = "/|)";
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
    case '<':
      consume();
      consume('<');
      consume('E');
      consume('O');
      consume('F');
      consume('>');
      consume('>');
      return std::make_unique<EndOfFileExpr>();
    case '(': {
      consume();
      std::unique_ptr<RegExpr> subExpr = parseExpr();
      consume(')');
      return subExpr;
    }
    case '"': {
      consume();
      std::unique_ptr<RegExpr> lhs = std::make_unique<CharacterExpr>(consume());
      while (!eof() && currentChar() != '"') {
        std::unique_ptr<RegExpr> rhs = std::make_unique<CharacterExpr>(consume());
        lhs = std::make_unique<ConcatenationExpr>(std::move(lhs), std::move(rhs));
      }
      consume('"');
      return lhs;
    }
    case '[':
      return parseCharacterClass();
    case '.':
      consume();
      return std::make_unique<DotExpr>();
    case '^':
      consume();
      return std::make_unique<BeginOfLineExpr>();
    case '$':
      consume();
      return std::make_unique<EndOfLineExpr>();
    default:
      return std::make_unique<CharacterExpr>(parseSingleCharacter());
  }
}

std::unique_ptr<RegExpr> RegExprParser::parseCharacterClass() {
  consume(); // '['
  const bool complement = consumeIf('^'); // TODO

  SymbolSet ss;
  parseCharacterClassFragment(ss);
  while (!eof() && currentChar() != ']')
    parseCharacterClassFragment(ss);

  if (complement)
    ss.complement();

  consume(']');
  return std::make_unique<CharacterClassExpr>(std::move(ss));
}

void RegExprParser::parseNamedCharacterClass(SymbolSet& ss) {
  consume('[');
  consume(':');
  std::string token;
  while (std::isalpha(currentChar())) {
    token += static_cast<char>(consume());
  }
  consume(':');
  consume(']');

  static const std::unordered_map<std::string_view, std::function<void()>> names = {
    {"alnum", [&]() {
      for (Symbol c = 'a'; c <= 'z'; c++) ss.insert(c);
      for (Symbol c = 'A'; c <= 'Z'; c++) ss.insert(c);
      for (Symbol c = '0'; c <= '9'; c++) ss.insert(c);
    }},
    {"alpha", [&]() {
      for (Symbol c = 'a'; c <= 'z'; c++) ss.insert(c);
      for (Symbol c = 'A'; c <= 'Z'; c++) ss.insert(c);
    }},
    {"blank", [&]() {
      ss.insert(' ');
      ss.insert('\t');
    }},
    {"cntrl", [&]() {
      for (Symbol c = 0; c <= 255; c++)
        if (std::iscntrl(c))
          ss.insert(c);
    }},
    {"digit", [&]() {
      for (Symbol c = '0'; c <= '9'; c++)
        ss.insert(c);
    }},
    {"graph", [&]() {
      for (Symbol c = 0; c <= 255; c++)
        if (std::isgraph(c))
          ss.insert(c);
    }},
    {"lower", [&]() {
      for (Symbol c = 'a'; c <= 'z'; c++)
        ss.insert(c);
    }},
    {"print", [&]() {
      for (Symbol c = 0; c <= 255; c++)
        if (std::isprint(c) || c == ' ')
          ss.insert(c);
    }},
    {"punct", [&]() {
      for (Symbol c = 0; c <= 255; c++)
        if (std::ispunct(c))
          ss.insert(c);
    }},
    {"space", [&]() {
      for (Symbol c : "\f\n\r\t\v")
        ss.insert(c);
    }},
    {"upper", [&]() {
      for (Symbol c = 'A'; c <= 'Z'; c++)
        ss.insert(c);
    }},
    {"xdigit", [&]() {
      for (Symbol c = '0'; c <= '9'; c++) ss.insert(c);
      for (Symbol c = 'a'; c <= 'f'; c++) ss.insert(c);
      for (Symbol c = 'A'; c <= 'F'; c++) ss.insert(c);
    }},
  };

  if (auto i = names.find(token); i != names.end())
    i->second();
  else
    throw UnexpectedToken{line_, column_, token, "<valid character class>"};
}

Symbol RegExprParser::parseSingleCharacter() {
  if (currentChar() != '\\')
    return consume();

  consume(); // consumes escape character
  switch (currentChar()) {
    case 'a':
      consume();
      return '\a';
    case 'b':
      consume();
      return '\b';
    case 'f':
      consume();
      return '\f';
    case 'n':
      consume();
      return '\n';
    case 'r':
      consume();
      return '\r';
    case 's':
      consume();
      return ' ';
    case 't':
      consume();
      return '\t';
    case 'v':
      consume();
      return '\v';
    case 'x': {
      consume();

      char buf[3];
      buf[0] = consume();
      if (!std::isxdigit(buf[0]))
        throw UnexpectedToken{line_, column_, std::string(1, buf[0]), "[0-9a-fA-F]"};
      buf[1] = consume();
      if (!std::isxdigit(buf[1]))
        throw UnexpectedToken{line_, column_, std::string(1, buf[1]), "[0-9a-fA-F]"};
      buf[2] = 0;

      return static_cast<Symbol>(strtoul(buf, nullptr, 16));
    }
    case '0': {
      const Symbol x0 = consume();
      if (!(currentChar() >= '1' && currentChar() <= '7'))
        return '\0';

      // octal value (\DDD)
      char buf[4];
      buf[0] = x0;
      buf[1] = consume();
      if (!(buf[1] >= '0' && buf[1] <= '7'))
        throw UnexpectedToken{line_, column_, std::string(1, buf[1]), "[0-7]"};
      buf[2] = consume();
      if (!(buf[2] >= '0' && buf[2] <= '7'))
        throw UnexpectedToken{line_, column_, std::string(1, buf[2]), "[0-7]"};
      buf[3] = '\0';

      return static_cast<Symbol>(strtoul(buf, nullptr, 8));
    }
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7': {
      // octal value (\DDD)
      char buf[4];
      buf[0] = consume();
      if (!(buf[0] >= '0' && buf[0] <= '7'))
        throw UnexpectedToken{line_, column_, std::string(1, buf[0]), "[0-7]"};
      buf[1] = consume();
      if (!(buf[1] >= '0' && buf[1] <= '7'))
        throw UnexpectedToken{line_, column_, std::string(1, buf[1]), "[0-7]"};
      buf[2] = consume();
      if (!(buf[2] >= '0' && buf[2] <= '7'))
        throw UnexpectedToken{line_, column_, std::string(1, buf[2]), "[0-7]"};
      buf[3] = '\0';

      return static_cast<Symbol>(strtoul(buf, nullptr, 8));
    }
    case '"':
    case '$':
    case '(':
    case ')':
    case '*':
    case '+':
    case ':':
    case '?':
    case '[':
    case '\'':
    case '\\':
    case ']':
    case '^':
    case '{':
    case '}':
      return consume();
    default: {
      throw UnexpectedToken{line_, column_, 
        fmt::format("'{}'", static_cast<char>(currentChar())),
        "<escape sequence character>"};
    }
  }
}

void RegExprParser::parseCharacterClassFragment(SymbolSet& ss) {
  // parse [:named:]
  if (currentChar() == '[') {
    parseNamedCharacterClass(ss);
    return;
  }

  // parse single char (A) or range (A-Z)
  const Symbol c1 = parseSingleCharacter();
  if (currentChar() != '-') {
    ss.insert(c1);
    return;
  }

  consume(); // consume '-'
  const Symbol c2 = parseSingleCharacter();

  for (Symbol c_i = c1; c_i <= c2; c_i++)
    ss.insert(c_i);
}

} // namespace klex
