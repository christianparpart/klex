// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/Rule.h>
#include <fmt/format.h>
#include <stdexcept>
#include <istream>
#include <string>
#include <memory>

namespace klex {

class RuleParser {
 public:
  explicit RuleParser(std::unique_ptr<std::istream> input);

  RuleList parseRules();

  class UnexpectedChar;
  class UnexpectedToken;
  class InvalidRuleOption;

 private:
  Rule parseRule();
  std::string parseExpression();

 private:
  std::string consumeToken();
  void consumeSP();
  void consumeAssoc();
  void consumeSpace();
  char currentChar() const noexcept;
  char consumeChar(char ch);
  char consumeChar();
  bool eof() const noexcept;

 private:
  std::unique_ptr<std::istream> stream_;
  char currentChar_;
  unsigned int line_;
  unsigned int column_;
  unsigned int offset_;
  int nextTag_;
};

class RuleParser::UnexpectedToken : public std::runtime_error {
 public:
  UnexpectedToken(unsigned offset, char actual, std::string expected)
      : std::runtime_error{fmt::format("{}: Unexpected token {}, expected <{}> instead.",
          offset, actual, expected)},
        offset_{offset},
        actual_{std::move(actual)},
        expected_{std::move(expected)} {}

  unsigned offset() const noexcept { return offset_; }
  char actual() const noexcept { return actual_; }
  const std::string& expected() const noexcept { return expected_; }

 private:
  unsigned offset_;
  unsigned line_;
  unsigned column_;
  char actual_;
  std::string expected_;
};

class RuleParser::UnexpectedChar : public std::runtime_error {
 public:
  UnexpectedChar(unsigned int line, unsigned int column, char actual, char expected)
      : std::runtime_error{fmt::format("[{}:{}] Unexpected char {}, expected {} instead.",
          line, column, actual, expected)},
        line_{line},
        column_{column},
        actual_{actual},
        expected_{expected} {}

  unsigned int line() const noexcept { return line_; }
  unsigned int column() const noexcept { return column_; }
  char actual() const noexcept { return actual_; }
  char expected() const noexcept { return expected_; }

 private:
  unsigned int line_;
  unsigned int column_;
  char actual_;
  char expected_;
};

class RuleParser::InvalidRuleOption : public std::runtime_error {
 public:
  InvalidRuleOption(unsigned offset, std::string option)
      : std::runtime_error{fmt::format("{}: Invalid rule option \"{}\".",
          offset, option)},
        offset_{offset},
        option_{option} {}

  unsigned offset() const noexcept { return offset_; }
  const std::string& option() const noexcept { return option_; }

 private:
  unsigned offset_;
  std::string option_;
};

} // namespace klex
