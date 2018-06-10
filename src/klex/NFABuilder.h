// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/Alphabet.h>
#include <klex/NFA.h>
#include <klex/RegExpr.h>

#include <fmt/format.h>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <tuple>
#include <vector>

namespace klex {

class DFA;

/*!
 * Generates a finite automaton from the given input (a regular expression).
 */
class NFABuilder : public RegExprVisitor {
 public:
  explicit NFABuilder()
      : fa_{} {}

  NFA construct(const RegExpr* re);

 private:
  void visit(AlternationExpr& alternationExpr) override;
  void visit(ConcatenationExpr& concatenationExpr) override;
  void visit(CharacterExpr& characterExpr) override;
  void visit(ClosureExpr& closureExpr) override;

 private:
  NFA fa_;
};

} // namespace klex
