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

  for (const TransitionMap& transitions : states_)
    for (const std::pair<Symbol, StateIdVec>& t : transitions)
      alphabet.insert(t.first);

  return alphabet;
}

NFA NFA::clone() const {
  return *this;
}

StateId NFA::createState() {
  states_.emplace_back();
  return states_.size() - 1;
}

StateId NFA::createState(Tag acceptTag) {
  StateId id = createState();
  acceptTags_[id] = acceptTag;
  return id;
}

NFA& NFA::concatenate(NFA rhs) {
  StateId base = states_.size();
  for (auto& t : rhs.states_) {
  }

  // ------------------------------------------------------------------------------
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
  const bool accept = s->tag() != 0 || s->isAccepting();
  const size_t id = registry.size();

  v.visitNode(id, start, accept);
  registry[s] = id;

  for (const Edge& edge : s->transitions()) {
    if (registry.find(edge.state) == registry.end()) {
      visit(v, edge.state, registry);
    }
    v.visitEdge(id, registry[edge.state], edge.symbol);
  }

  for (const Edge& edge : s->transitions()) {
    v.endVisitEdge(id, registry[edge.state]);
  }
}

} // namespace klex
