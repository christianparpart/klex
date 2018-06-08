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

// {{{ helper
static StateId nextId(const OwnedStateSet& states) {
  StateId id = 0;

  for (const std::unique_ptr<State>& s : states)
    if (id <= s->id())
      id = s->id() + 1;

  return id;
}
// }}}

ThompsonConstruct::ThompsonConstruct(DFA dfa)
    : ThompsonConstruct{} {
  const auto acceptStates = dfa.acceptStates();
  std::tuple<OwnedStateSet, State*> owned = dfa.release();
  states_ = std::move(std::get<0>(owned));
  initialState_ = std::get<1>(owned);
  nextId_ = nextId(states_);

  if (acceptStates.size() > 1) {
    State* newEnd = createState();
    for (State* a : acceptStates) {
      a->setAccept(false);
      a->linkTo(newEnd);
    }
    newEnd->setAccept(true);
  }
}

Alphabet ThompsonConstruct::alphabet() const {
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

void ThompsonConstruct::setTag(Tag tag) {
  for (const std::unique_ptr<State>& s : states_) {
    assert(s->tag() == 0 && "State.tag() must not be set more than once");
    s->setTag(tag);
  }
}

ThompsonConstruct ThompsonConstruct::clone() const {
  ThompsonConstruct output;

  // clone states
  for (const std::unique_ptr<State>& s : states_) {
    State* u = output.createState(s->id(), s->isAccepting(), s->tag());
    if (s.get() == initialState()) {
      output.initialState_ = u;
    }
  }

  // map links
  for (const std::unique_ptr<State>& s : states_) {
    State* u = output.findState(s->id());
    for (const Edge& transition : s->transitions()) {
      State* v = output.findState(transition.state->id());
      u->linkTo(transition.symbol, v);
      // findState(s->id())->linkTo(t.symbol, findState(t.state->id()));
    }
  }

  output.nextId_ = nextId_;

  return output;
}

State* ThompsonConstruct::acceptState() const {
  for (const std::unique_ptr<State>& s : states_)
    if (s->isAccepting())
      return s.get();

  fprintf(stderr, "Internal Bug! Thompson's Consruct without an end state.\n");
  abort();
}

// TODO: Remove me?
// std::tuple<OwnedStateSet, State*> ThompsonConstruct::release() {
//   auto t = std::make_tuple(std::move(states_), initialState_);
//   initialState_ = nullptr;
//   return t;
// }

State* ThompsonConstruct::createState() {
  return createState(nextId_++, false, 0);
}

State* ThompsonConstruct::createState(StateId id, bool accepting, Tag tag) {
  if (findState(id) != nullptr)
    throw std::invalid_argument{fmt::format("StateId: {}", id)};

  states_.emplace_back(std::make_unique<State>(id, accepting, tag));
  return states_.back().get();
}

State* ThompsonConstruct::findState(StateId id) const {
  for (const std::unique_ptr<State>& s : states_)
    if (s->id() == id)
      return s.get();

  return nullptr;
}

ThompsonConstruct& ThompsonConstruct::concatenate(ThompsonConstruct rhs) {
  acceptState()->linkTo(rhs.initialState_);
  acceptState()->setAccept(false);

  // renumber first with given base
  for (const std::unique_ptr<State>& s : rhs.states_) {
    VERIFY_STATE_AVAILABILITY(nextId_, states_);
    s->setId(nextId_++);
  }

  for (std::unique_ptr<State>& s : rhs.states_)
    states_.emplace_back(std::move(s));

  return *this;
}

ThompsonConstruct& ThompsonConstruct::alternate(ThompsonConstruct other) {
  State* newStart = createState();
  newStart->linkTo(initialState_);
  newStart->linkTo(other.initialState_);

  State* newEnd = createState();
  acceptState()->linkTo(newEnd);
  other.acceptState()->linkTo(newEnd);

  initialState_ = newStart;
  acceptState()->setAccept(false);
  other.acceptState()->setAccept(false);
  newEnd->setAccept(true);

  // renumber first with given base
  for (const std::unique_ptr<State>& s : other.states_) {
    VERIFY_STATE_AVAILABILITY(nextId_, states_);
    s->setId(nextId_++);
  }

  for (std::unique_ptr<State>& s : other.states_)
    states_.emplace_back(std::move(s));

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
  for (const std::unique_ptr<State>& s : states_)
    for (const Edge& edge : s->transitions())
      if (edge.state == t && edge.symbol == EpsilonTransition)
        return true;

  return false;
}

} // namespace klex
