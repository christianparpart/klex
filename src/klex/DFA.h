// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/Alphabet.h>
#include <klex/State.h>
#include <klex/util/UnboxedRange.h>

namespace klex {

class ThompsonConstruct;

/**
 * Represents a deterministic finite automaton.
 */
class DFA {
 public:
  DFA(const DFA& other) : DFA{} { *this = other; }
  DFA& operator=(const DFA& other);
  DFA(DFA&&) = default;
  DFA& operator=(DFA&&) = default;
  ~DFA() = default;

  DFA() : states_{}, initialState_{nullptr} {}

  //! constructs a DFA from a given @p nfa.
  static DFA construct(ThompsonConstruct nfa);

  //! Retrieves the alphabet of this finite automaton.
  Alphabet alphabet() const;

  //! Retrieves the initial state.
  State* initialState() const { return initialState_; }

  //! Retrieves the list of available states.
  auto states() const { return util::unbox(states_); }

  //! Retrieves the list of accepting states.
  StateSet acceptStates() const;

  const OwnedStateSet& ownedStates() const noexcept { return states_; }

  //! Finds State by its @p id.
  State* findState(StateId id) const;

  //! Constructs a minimized version of this DFA.
  DFA minimize() const;

  //! Releases ownership of internal data structures to the callers responsibility.
  std::tuple<OwnedStateSet, State*> release();

  void renumber();

 private:
  void renumber(State* s, std::set<State*>* registry);
  State* createState(StateId expectedId);
  void setInitialState(State* state);

 private:
  OwnedStateSet states_;
  State* initialState_;
};

} // namespace klex {
