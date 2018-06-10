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
  int id { 0 };

  v.start();
  visit(v, initialState_, &id);
  v.end();
}

void DFA::visit(DotVisitor& v, const State* s, int* id) const {
  const bool start = s == initialState_;
  const bool accept = s->isAccepting();
  const int sourceId = *id;

  v.visitNode(*id, start, accept);
  *id = *id + 1;

  for (const Edge& edge : s->transitions()) {
    const int targetId = *id;
    const std::string edgeText = prettySymbol(edge.symbol);

    visit(v, edge.state, id);
    v.visitEdge(sourceId, targetId, edgeText);
  }
}

} // namespace klex
