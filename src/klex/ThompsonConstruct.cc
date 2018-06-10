// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/ThompsonConstruct.h>
#include <klex/DFA.h>
#include <iostream>

namespace klex {

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

Alphabet ThompsonConstruct::alphabet() const {
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

ThompsonConstruct ThompsonConstruct::clone() const {
  ThompsonConstruct output;

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

// TODO: Remove me?
// std::tuple<OwnedStateSet, State*, State*> ThompsonConstruct::release() {
//   auto t = std::make_tuple(std::move(states_), initialState_, acceptState_);
//   initialState_ = nullptr;
//   acceptState_ = nullptr;
//   return t;
// }

State* ThompsonConstruct::createState() {
  return states_.create();
}

State* ThompsonConstruct::createState(bool accepting, Tag acceptTag) {
  return states_.create(accepting, acceptTag);
}

State* ThompsonConstruct::createState(StateId id, bool accepting, Tag acceptTag) {
  assert(id == states_.nextId());
  return states_.create(accepting, acceptTag);
}

ThompsonConstruct& ThompsonConstruct::concatenate(ThompsonConstruct rhs) {
  acceptState()->linkTo(rhs.initialState_);
  acceptState()->setAccept(false);

  states_.append(std::move(rhs.states_));

  rhs.initialState_ = nullptr;
  rhs.acceptState_ = nullptr;

  return *this;
}

ThompsonConstruct& ThompsonConstruct::alternate(ThompsonConstruct rhs) {
  State* newStart = createState();
  states_.append(std::move(rhs.states_));
  State* newEnd = createState();

  newStart->linkTo(initialState_);
  newStart->linkTo(rhs.initialState_);

  acceptState_->linkTo(newEnd);
  rhs.acceptState_->linkTo(newEnd);

  initialState_ = newStart;
  acceptState_ = newEnd;

  acceptState_->setAccept(false);
  rhs.acceptState_->setAccept(false);
  newEnd->setAccept(true);

  rhs.initialState_ = nullptr;
  rhs.acceptState_ = nullptr;

  return *this;
}

ThompsonConstruct& ThompsonConstruct::optional() {
  State* newStart = createState();
  State* newEnd = createState();

  newStart->linkTo(initialState_);
  newStart->linkTo(newEnd);
  acceptState()->linkTo(newEnd);

  initialState_ = newStart;
  acceptState()->setAccept(false);
  newEnd->setAccept(true);

  return *this;
}

ThompsonConstruct& ThompsonConstruct::recurring() {
  // {0, inf}
  State* newStart = createState();
  State* newEnd = createState();
  newStart->linkTo(initialState_);
  newStart->linkTo(newEnd);
  acceptState()->linkTo(initialState_);
  acceptState()->linkTo(newEnd);

  initialState_ = newStart;
  acceptState()->setAccept(false);
  newEnd->setAccept(true);

  return *this;
}

ThompsonConstruct& ThompsonConstruct::positive() {
  return concatenate(std::move(clone().recurring()));
}

ThompsonConstruct& ThompsonConstruct::times(unsigned factor) {
  assert(factor != 0);

  if (factor == 1)
    return *this;

  ThompsonConstruct base = clone();
  for (unsigned n = 2; n <= factor; ++n)
    concatenate(base.clone());

  return *this;
}

ThompsonConstruct& ThompsonConstruct::repeat(unsigned minimum, unsigned maximum) {
  assert(minimum <= maximum);

  ThompsonConstruct factor = clone();

  times(minimum);
  for (unsigned n = minimum + 1; n <= maximum; n++)
    alternate(std::move(factor.clone().times(n)));

  return *this;
}

bool ThompsonConstruct::isReceivingEpsilon(const State* t) const noexcept {
  for (const State* s : states_)
    for (const Edge& edge : s->transitions())
      if (edge.state == t && edge.symbol == EpsilonTransition)
        return true;

  return false;
}

} // namespace klex
