// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/RegExprExporter.h>
#include <klex/DFA.h>

namespace klex { 

DFA RegExprExporter::generate(const RegExpr* re) {
  ThompsonConstruct nfa = construct(re);
  return DFA::construct(std::move(nfa));
}

ThompsonConstruct RegExprExporter::construct(const RegExpr* re) {
  const_cast<RegExpr*>(re)->accept(*this);

  return std::move(fa_);
}

void RegExprExporter::visit(AlternationExpr& alternationExpr) {
  ThompsonConstruct lhs = construct(alternationExpr.leftExpr());
  ThompsonConstruct rhs = construct(alternationExpr.rightExpr());
  lhs.alternate(std::move(rhs));
  fa_ = std::move(lhs);
}

void RegExprExporter::visit(ConcatenationExpr& concatenationExpr) {
  ThompsonConstruct lhs = construct(concatenationExpr.leftExpr());
  ThompsonConstruct rhs = construct(concatenationExpr.rightExpr());
  lhs.concatenate(std::move(rhs));
  fa_ = std::move(lhs);
}

void RegExprExporter::visit(CharacterExpr& characterExpr) {
  fa_ = ThompsonConstruct{characterExpr.value()};
}

void RegExprExporter::visit(ClosureExpr& closureExpr) {
  const unsigned xmin = closureExpr.minimumOccurrences();
  const unsigned xmax = closureExpr.maximumOccurrences();
  constexpr unsigned Infinity = std::numeric_limits<unsigned>::max();

  if (xmin == 0 && xmax == 1)
    fa_ = std::move(construct(closureExpr.subExpr()).optional());
  else if (xmin == 0 && xmax == Infinity)
    fa_ = std::move(construct(closureExpr.subExpr()).recurring());
  else if (xmin == 1 && xmax == Infinity)
    fa_ = std::move(construct(closureExpr.subExpr()).positive());
  else if (xmin < xmax)
    fa_ = std::move(construct(closureExpr.subExpr()).repeat(xmin, xmax));
  else if (xmin == xmax)
    fa_ = std::move(construct(closureExpr.subExpr()).times(xmin));
  else
    throw std::invalid_argument{"closureExpr"};
}

} // namespace klex
