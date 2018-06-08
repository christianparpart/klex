// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/fa.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <deque>
#include <functional>
#include <iostream>
#include <sstream>
#include <vector>

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

namespace klex::fa {

// {{{ utils

std::string _groupCharacterClassRanges(std::vector<Symbol> chars) {
  std::sort(chars.begin(), chars.end());
  std::stringstream sstr;
  for (Symbol c : chars)
    sstr << prettySymbol(c);
  return sstr.str();
}
// }}}

// {{{ FiniteAutomaton
FiniteAutomaton& FiniteAutomaton::operator=(const FiniteAutomaton& other) {
  states_.clear();
  for (State* s : other.states())
    createState(s->id())->setAccept(s->isAccepting());

  // map links
  for (const std::unique_ptr<State>& s : other.states_)
    for (const Edge& t : s->transitions())
      findState(s->id())->linkTo(t.symbol, findState(t.state->id()));

  initialState_ = findState(other.initialState()->id());

  return *this;
}

void FiniteAutomaton::renumber() {
  std::set<State*> registry;
  renumber(initialState_, &registry);
}

void FiniteAutomaton::renumber(State* s, std::set<State*>* registry) {
  StateId id = registry->size();
  VERIFY_STATE_AVAILABILITY(id, *registry);
  s->setId(id);
  registry->insert(s);
  for (const Edge& transition : s->transitions()) {
    if (registry->find(transition.state) == registry->end()) {
      renumber(transition.state, registry);
    }
  }
}

State* FiniteAutomaton::createState() {
  return createState(nextId(states_));
}

State* FiniteAutomaton::createState(StateId id) {
  for (State* s : states())
    if (s->id() == id)
      throw std::invalid_argument{fmt::format("StateId: {}", id)};

  return states_.insert(std::make_unique<State>(id)).first->get();
}

void FiniteAutomaton::setInitialState(State* s) {
  // TODO: assert (s is having no predecessors)
  initialState_ = s;
}
// }}} 

} // namespace klex::fa
