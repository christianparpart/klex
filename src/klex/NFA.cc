// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/NFA.h>
#include <klex/DFA.h>
#include <klex/DotVisitor.h>
#include <iostream>

namespace klex {

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

Alphabet NFA::alphabet() const {
  Alphabet alphabet;
  for (const State* state : states_) {
    for (const Edge& transition : state->transitions()) {
      if (transition.symbol != EpsilonTransition) {
        alphabet.insert(transition.symbol);
      }
    }
  }
  return alphabet;
}

NFA NFA::clone() const {
  NFA output;

  // clone states
  for (const State* s : states_) {
    State* u = output.createState(s->isAccepting(), s->tag());
    if (s == initialState_) {
      output.initialState_ = u;
    }
    if (s == acceptState_) {
      output.acceptState_ = u;
    }
  }

  // map links
  for (const State* s : states_) {
    State* u = output.findState(s->id());
    for (const Edge& transition : s->transitions()) {
      State* v = output.findState(transition.state->id());
      u->linkTo(transition.symbol, v);
      // findState(s->id())->linkTo(t.symbol, findState(t.state->id()));
    }
  }

  return output;
}

State* NFA::createState() {
  return states_.create();
}

State* NFA::createState(bool accepting, Tag acceptTag) {
  return states_.create(accepting, acceptTag);
}

State* NFA::createState(StateId id, bool accepting, Tag acceptTag) {
  assert(id == states_.nextId());
  return states_.create(accepting, acceptTag);
}

NFA& NFA::concatenate(NFA rhs) {
  acceptState_->linkTo(rhs.initialState_);
  acceptState_->setAccept(false);

  acceptState_ = rhs.acceptState_;

  states_.append(std::move(rhs.states_));

  rhs.initialState_ = nullptr;
  rhs.acceptState_ = nullptr;

  return *this;
}

NFA& NFA::alternate(NFA rhs) {
  State* newStart = createState();
  states_.append(std::move(rhs.states_));
  State* newEnd = createState();

  newStart->linkTo(initialState_);
  newStart->linkTo(rhs.initialState_);

  acceptState_->linkTo(newEnd);
  rhs.acceptState_->linkTo(newEnd);

  acceptState_->setAccept(false);
  rhs.acceptState_->setAccept(false);
  newEnd->setAccept(true);

  initialState_ = newStart;
  acceptState_ = newEnd;

  rhs.initialState_ = nullptr;
  rhs.acceptState_ = nullptr;

  return *this;
}

NFA& NFA::optional() {
  State* newStart = createState();
  State* newEnd = createState();

  newStart->linkTo(initialState_);
  newStart->linkTo(newEnd);
  acceptState_->linkTo(newEnd);

  acceptState_->setAccept(false);
  newEnd->setAccept(true);

  initialState_ = newStart;
  acceptState_ = newEnd;

  return *this;
}

NFA& NFA::recurring() {
  // {0, inf}
  State* newStart = createState();
  State* newEnd = createState();

  newStart->linkTo(initialState_);
  newStart->linkTo(newEnd);

  acceptState_->linkTo(initialState_);
  acceptState_->linkTo(newEnd);

  acceptState_->setAccept(false);
  newEnd->setAccept(true);

  initialState_ = newStart;
  acceptState_ = newEnd;

  return *this;
}

NFA& NFA::positive() {
  return concatenate(std::move(clone().recurring()));
}

NFA& NFA::times(unsigned factor) {
  assert(factor != 0);

  if (factor == 1)
    return *this;

  NFA base = clone();
  for (unsigned n = 2; n <= factor; ++n)
    concatenate(base.clone());

  return *this;
}

NFA& NFA::repeat(unsigned minimum, unsigned maximum) {
  assert(minimum <= maximum);

  NFA factor = clone();

  times(minimum);
  for (unsigned n = minimum + 1; n <= maximum; n++)
    alternate(std::move(factor.clone().times(n)));

  return *this;
}

bool NFA::isReceivingEpsilon(const State* t) const noexcept {
  for (const State* s : states_)
    for (const Edge& edge : s->transitions())
      if (edge.state == t && edge.symbol == EpsilonTransition)
        return true;

  return false;
}

void NFA::visit(DotVisitor& v) const {
  v.start();
#if 0
  for (const State* s : states_) {
    const bool start = s == initialState_;
    const bool accept = s->isAccepting();

    v.visitNode(s->id(), start, accept);

    for (const Edge& edge : s->transitions()) {
      const std::string edgeText = prettySymbol(edge.symbol);
      v.visitEdge(s->id(), edge.state->id(), edgeText);
    }
  }
#else
  std::unordered_map<const State*, size_t> registry;
  visit(v, initialState_, registry);
#endif
  v.end();
}

void NFA::visit(DotVisitor& v, const State* s, std::unordered_map<const State*, size_t>& registry) const {
  const bool start = s == initialState_;
  const bool accept = s->isAccepting();
  const size_t id = registry.size();

  v.visitNode(id, start, accept);
  registry[s] = id;

  for (const Edge& edge : s->transitions()) {
    const std::string edgeText = prettySymbol(edge.symbol);
    if (registry.find(edge.state) == registry.end()) {
      visit(v, edge.state, registry);
    }
    v.visitEdge(id, registry[edge.state], edgeText);
  }
}

} // namespace klex
