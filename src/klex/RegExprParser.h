// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <memory>
#include <string_view>

#include <fmt/format.h>

namespace klex {

class RegExpr;

class RegExprParser {
 public:
  RegExprParser();

  std::unique_ptr<RegExpr> parse(std::string_view expr);

  class UnexpectedToken : public std::runtime_error {
   public:
    UnexpectedToken(int actual, int expected)
        : std::runtime_error{fmt::format("Unexpected token {}. Expected {} instead.",
                                         actual == -1 ? "EOF"
                                                      : fmt::format("{}", static_cast<char>(actual)),
                                         static_cast<char>(expected))}
    {}
  };

 private:
  int currentChar() const;
  bool eof() const noexcept { return currentChar() == -1; }
  bool consumeIf(int ch);
  void consume(int ch);
  int consume();
  unsigned parseInt();

  std::unique_ptr<RegExpr> parseExpr();                   // alternation
  std::unique_ptr<RegExpr> parseAlternation();            // concatenation ('|' concatenation)*
  std::unique_ptr<RegExpr> parseConcatenation();          // closure (closure)*
  std::unique_ptr<RegExpr> parseClosure();                // atom ['*' | '?' | '{' NUM [',' NUM] '}']
  std::unique_ptr<RegExpr> parseAtom();                   // character | characterClass | '(' expr ')'
  std::unique_ptr<RegExpr> parseCharacterClass();         // '[' characterClassFragment+ ']'
  std::unique_ptr<RegExpr> parseCharacterClassFragment(); // character | character '-' character

 private:
  std::string_view input_;
  std::string_view::iterator currentChar_;
};

} // namespace klex
