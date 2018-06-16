// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/DFA.h>
#include <klex/DotVisitor.h>
#include <klex/NFA.h>
#include <iostream>
#include <deque>
#include <map>
#include <sstream>
#include <vector>

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

namespace klex {

Alphabet DFA::alphabet() const {
  Alphabet alphabet;
  for (const State& state : states_)
    for (const std::pair<Symbol, StateId>& t : state.transitions)
      alphabet.insert(t.first);

  return std::move(alphabet);
}

std::vector<StateId> DFA::acceptStates() const {
  std::vector<StateId> states;
  states.reserve(acceptTags_.size());
  std::for_each(acceptTags_.begin(), acceptTags_.end(), [&](const auto& s) { states.push_back(s.first); });

  return std::move(states);
}

// --------------------------------------------------------------------------

void DFA::createStates(size_t count) {
  states_.resize(states_.size() + count);
}

StateId DFA::createState() {
  states_.emplace_back();
  return states_.size() - 1;
}

void DFA::setInitialState(StateId s) {
  // TODO: assert (s is having no predecessors)
  initialState_ = s;
}

void DFA::setTransition(StateId from, Symbol symbol, StateId to) {
  const State& s = states_[from];
  if (auto i = s.transitions.find(symbol); i != s.transitions.end()) {
    std::cerr << fmt::format("n{} --({})--> n{} attempted to overwrite with (n{})\n",
        from, symbol, i->second, to);
    abort();
  }

  states_[from].transitions[symbol] = to;
}

void DFA::visit(DotVisitor& v) const {
  v.start();

  // STATE: initial
  v.visitNode(initialState_, true, isAccepting(initialState_));

  // STATE: accepting
  for (StateId s : acceptStates())
    if (s != initialState_)
      v.visitNode(s, s == initialState_, true);

  // STATE: any other
  for (StateId s : stateIds())
    if (s != initialState_ && !isAccepting(s))
      v.visitNode(s, false, false);

  // TRANSITIONS
  for (StateId s = 0, sE = size(); s != sE; ++s) {
    const TransitionMap& T = states_[s].transitions;
    std::for_each(T.begin(), T.end(), [&](const auto& t) { v.visitEdge(s, t.second, t.first); });
    std::for_each(T.begin(), T.end(), [&](const auto& t) { v.endVisitEdge(s, t.second); });
  }
  v.end();
}

} // namespace klex
