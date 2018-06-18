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
class SymbolSet;

class RegExprParser {
 public:
  RegExprParser();

  std::unique_ptr<RegExpr> parse(std::string_view expr, int line, int column);

  std::unique_ptr<RegExpr> parse(std::string_view expr) {
    return parse(std::move(expr), 1, 1);
  }

  class UnexpectedToken : public std::runtime_error {
   public:
    UnexpectedToken(unsigned int line, unsigned int column, int actual, int expected)
        : std::runtime_error{fmt::format("[{}:{}] Unexpected token {}. Expected {} instead.",
                                         line, column,
                                         actual == -1 ? "EOF"
                                                      : fmt::format("{}", static_cast<char>(actual)),
                                         static_cast<char>(expected))},
          line_{line},
          column_{column},
          actual_{actual},
          expected_{expected}
    {}

    unsigned int line() const noexcept { return line_; }
    unsigned int column() const noexcept { return column_; }
    int actual() const noexcept { return actual_; }
    int expected() const noexcept { return expected_; }

   private:
    unsigned int line_;
    unsigned int column_;
    int actual_;
    int expected_;
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
  void parseCharacterClassFragment(SymbolSet& ss);        // character | character '-' character

 private:
  std::string_view input_;
  std::string_view::iterator currentChar_;
  unsigned int line_;
  unsigned int column_;
};

} // namespace klex
