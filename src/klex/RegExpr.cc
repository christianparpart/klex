// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

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

void AlternationExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string AlternationExpr::to_string() const {
  std::stringstream sstr;

  if (precedence() > left_->precedence()) {
    sstr << '(' << left_->to_string() << ')';
  } else
    sstr << left_->to_string();

  sstr << "|";

  if (precedence() > right_->precedence()) {
    sstr << '(' << right_->to_string() << ')';
  } else
    sstr << right_->to_string();

  return sstr.str();
}

void ConcatenationExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string ConcatenationExpr::to_string() const {
  std::stringstream sstr;

  if (precedence() > left_->precedence()) {
    sstr << '(' << left_->to_string() << ')';
  } else
    sstr << left_->to_string();

  if (precedence() > right_->precedence()) {
    sstr << '(' << right_->to_string() << ')';
  } else
    sstr << right_->to_string();

  return sstr.str();
}

void CharacterExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string CharacterExpr::to_string() const {
  return std::string(1, value_);
}

void EndOfFileExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string EndOfFileExpr::to_string() const {
  return "<<EOF>>";
}

void BeginOfLineExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string BeginOfLineExpr::to_string() const {
  return "^";
}

void EndOfLineExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string EndOfLineExpr::to_string() const {
  return "$";
}

std::string CharacterClassExpr::to_string() const {
  return value_.to_string();
}

void CharacterClassExpr::accept(RegExprVisitor& visitor) {
  visitor.visit(*this);
}

void DotExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string DotExpr::to_string() const {
  return ".";
}

void ClosureExpr::accept(RegExprVisitor& visitor) {
  return visitor.visit(*this);
}

std::string ClosureExpr::to_string() const {
  std::stringstream sstr;

  // TODO: optimize superfluous ()'s
  if (precedence() > subExpr_->precedence())
    sstr << '(' << subExpr_->to_string() << ')';
  else
    sstr << subExpr_->to_string();

  if (minimumOccurrences_ == 0 && maximumOccurrences_ == 1)
    sstr << '?';
  else if (minimumOccurrences_ == 0 && maximumOccurrences_ == std::numeric_limits<unsigned>::max())
    sstr << '*';
  else if (minimumOccurrences_ == 1 && maximumOccurrences_ == std::numeric_limits<unsigned>::max())
    sstr << '+';
  else
    sstr << '{' << minimumOccurrences_ << ',' << maximumOccurrences_ << '}';

  return sstr.str();
}

} // namespace klex
