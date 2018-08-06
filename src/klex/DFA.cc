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

void DFA::setInitialState(StateId s) {
  // TODO: assert (s is having no predecessors)
  initialState_ = s;
}

void DFA::setTransition(StateId from, Symbol symbol, StateId to) {
  const State& s = states_[from];
  if (auto i = s.transitions.find(symbol); i != s.transitions.end()) {
    printf("overwriting transition! %lu --(%s)--> %lu (new: %lu)\n", from, prettySymbol(symbol).c_str(), i->second, to);
  }
  // XXX assert(s.transitions.find(symbol) == s.transitions.end());
  states_[from].transitions[symbol] = to;
}

void DFA::removeTransition(StateId from, Symbol symbol) {
  State& s = states_[from];
  if (auto i = s.transitions.find(symbol); i != s.transitions.end()) {
    s.transitions.erase(i);
  }
}

StateId DFA::append(DFA other, StateId q0) {
  assert(other.initialState() == 0);

  other.prepareStateIds(states_.size(), q0);

  states_.reserve(size() + other.size() - 1);
  states_[q0] = other.states_[0];
  states_.insert(states_.end(), std::next(other.states_.begin()), other.states_.end());
  backtrackStates_.insert(other.backtrackStates_.begin(), other.backtrackStates_.end());
  acceptTags_.insert(other.acceptTags_.begin(), other.acceptTags_.end());

  return other.initialState();
}

void DFA::prepareStateIds(StateId baseId, StateId q0) {
  // adjust transition state IDs
  // traverse through each state's transition set
  //    traverse through each transition in the transition set
  //        traverse through each element and add BASE_ID

  auto transformId = [baseId, q0, this](StateId s) -> StateId {
    // we subtract 1, because we already have a slot for q0 elsewhere (pre-allocated)
    return s != initialState_ ? baseId + s - 1 : q0;
  };

  // for each state's transitions
  for (State& state : states_)
    for (std::pair<const Symbol, StateId>& t : state.transitions)
      t.second = transformId(t.second);

  AcceptMap remapped;
  for (auto& a : acceptTags_)
    remapped[transformId(a.first)] = a.second;
  acceptTags_ = std::move(remapped);

  BacktrackingMap backtracking;
  for (const auto& bt : backtrackStates_)
    backtracking[transformId(bt.first)] = transformId(bt.second);
  backtrackStates_ = std::move(backtracking);

  initialState_ = q0;
}

void DFA::visit(DotVisitor& v) const {
  v.start(initialState_);

  // STATE: initial
  v.visitNode(initialState_, true, isAccepting(initialState_));

  // STATE: accepting
  for (StateId s : acceptStates())
    if (s != initialState_)
      v.visitNode(s, false, true);

  // STATE: any other
  for (StateId s = 0, sE = lastState(); s != sE; ++s)
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
