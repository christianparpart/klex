// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/Alphabet.h>
#include <klex/State.h>
#include <map>

namespace klex {

class NFA;
class DFABuilder;
class DotVisitor;

/**
 * Represents a deterministic finite automaton.
 */
class DFA {
 public:
  // struct State {
  //   std::vector<StateId> states;
  //   std::map<Symbol, StateId> moves;
  // };
  // using StateVec = std::vector<State>;

  DFA(const DFA& other) : DFA{} { *this = other; }
  DFA& operator=(const DFA& other);
  DFA(DFA&&) = default;
  DFA& operator=(DFA&&) = default;
  ~DFA() = default;

  DFA() : states_{}, initialState_{nullptr} {}

  size_t size() const noexcept { return states_.size(); }

  //! Retrieves the alphabet of this finite automaton.
  Alphabet alphabet() const;

  //! Retrieves the initial state.
  State* initialState() const { return initialState_; }

  //! Retrieves the list of available states.
  const StateVec& states() const { return states_; }
  StateVec& states() { return states_; }

  //! Retrieves the list of accepting states.
  std::vector<const State*> acceptStates() const;
  std::vector<State*> acceptStates();

  /**
   * Traverses all states and edges in this NFA and calls @p visitor for each state & edge.
   *
   * Use this function to e.g. get a GraphViz dot-file drawn.
   */
  void visit(DotVisitor& visitor) const;

  void createStates(size_t count);
  State* createState();
  void setInitialState(State* state);
  State* findState(StateId id) { return states_.find(id); }

 private:
  StateVec states_;
  State* initialState_;
  //AcceptMap acceptTags_;
};

} // namespace klex
