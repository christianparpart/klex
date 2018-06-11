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
  for (const State* state : states()) {
    for (const Edge& transition : state->transitions()) {
      if (transition.symbol != EpsilonTransition) {
        alphabet.insert(transition.symbol);
      }
    }
  }
  return alphabet;
}

std::vector<const State*> DFA::acceptStates() const {
  std::vector<const State*> result;

  for (const State* s : states_)
    if (s->isAccepting())
      result.push_back(s);

  return result;
}

std::vector<State*> DFA::acceptStates() {
  std::vector<State*> result;

  for (State* s : states_)
    if (s->isAccepting())
      result.push_back(s);

  return result;
}

// --------------------------------------------------------------------------

void DFA::createStates(size_t count) {
  states_.reserve(states_.size() + count);

  for (size_t i = 0; i < count; ++i) {
    states_.create();
  }
}

State* DFA::createState() {
  return states_.create();
}

State* DFA::createState(StateId id) {
  assert(id == states_.nextId());
  return states_.create();
}

void DFA::setInitialState(State* s) {
  // TODO: assert (s is having no predecessors)
  initialState_ = s;
}

void DFA::visit(DotVisitor& v) const {
#if 1
  v.start();

  v.visitNode(initialState_->id(), true, initialState_->isAccepting());

  for (const State* s : acceptStates())
    if (s != initialState_)
      v.visitNode(s->id(), s == initialState_, true);

  for (const State* s : states_)
    if (s != initialState_ && !s->isAccepting())
      v.visitNode(s->id(), false, false);

  for (const State* s : states_) {
    for (const Edge& edge : s->transitions()) {
      v.visitEdge(s->id(), edge.state->id(), edge.symbol);
    }
    for (const Edge& edge : s->transitions()) {
      v.endVisitEdge(s->id(), edge.state->id());
    }
  }
  v.end();
#else
  v.start();
  for (const State* s : states_) {
    const bool start = s == initialState_;
    const bool accept = s->isAccepting();

    v.visitNode(s->id(), start, accept);

    for (const Edge& edge : s->transitions()) {
      const std::string edgeText = prettySymbol(edge.symbol);
      v.visitEdge(s->id(), edge.state->id(), edgeText);
    }
  }
  v.end();
#endif
}

} // namespace klex
