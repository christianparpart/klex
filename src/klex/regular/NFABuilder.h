// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/regular/Alphabet.h>
#include <klex/regular/NFA.h>
#include <klex/regular/RegExpr.h>

#include <fmt/format.h>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <string_view>
#include <tuple>
#include <vector>

namespace klex::regular {

class DFA;

/*!
 * Generates a finite automaton from the given input (a regular expression).
 */
class NFABuilder : public RegExprVisitor {
 public:
  explicit NFABuilder()
      : fa_{} {}

  NFA construct(const RegExpr* re, Tag tag);

 private:
  NFA construct(const RegExpr* re);
  void visit(LookAheadExpr& lookaheadExpr) override;
  void visit(ConcatenationExpr& concatenationExpr) override;
  void visit(AlternationExpr& alternationExpr) override;
  void visit(CharacterExpr& characterExpr) override;
  void visit(CharacterClassExpr& characterClassExpr) override;
  void visit(ClosureExpr& closureExpr) override;
  void visit(BeginOfLineExpr& eolExpr) override;
  void visit(EndOfLineExpr& eolExpr) override;
  void visit(EndOfFileExpr& eofExpr) override;
  void visit(DotExpr& dotExpr) override;
  void visit(EmptyExpr& emptyExpr) override;

 private:
  NFA fa_;
  std::optional<StateId> acceptState_;
};

} // namespace klex::regular
