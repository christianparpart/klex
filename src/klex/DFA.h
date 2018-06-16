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
#include <algorithm>

namespace klex {

class NFA;
class DFABuilder;
class DotVisitor;

/**
 * Represents a deterministic finite automaton.
 */
class DFA {
 public:
  using TransitionMap = std::map<Symbol, StateId>;
  struct State {
    std::vector<StateId> states;
    TransitionMap transitions;
  };
  using StateVec = std::vector<State>;

  DFA(const DFA& other) : DFA{} { *this = other; }
  DFA& operator=(const DFA& other);
  DFA(DFA&&) = default;
  DFA& operator=(DFA&&) = default;
  ~DFA() = default;

  DFA() : states_{}, initialState_{0}, acceptTags_{} {}

  size_t size() const noexcept { return states_.size(); }

  //! Retrieves the alphabet of this finite automaton.
  Alphabet alphabet() const;

  //! Retrieves the initial state.
  StateId initialState() const { return initialState_; }

  //! Retrieves the list of available states.
  const StateVec& states() const { return states_; }
  StateVec& states() { return states_; }

  StateIdVec stateIds() const {
    StateIdVec v;
    v.reserve(states_.size());
    for (size_t i = 0, e = states_.size(); i != e; ++i)
      v.push_back(i); // funny, I know
    return std::move(v);
  }

  //! Retrieves the list of accepting states.
  std::vector<StateId> acceptStates() const;

  /**
   * Traverses all states and edges in this NFA and calls @p visitor for each state & edge.
   *
   * Use this function to e.g. get a GraphViz dot-file drawn.
   */
  void visit(DotVisitor& visitor) const;

  void createStates(size_t count);
  StateId createState();

  void setInitialState(StateId state);

  const TransitionMap& stateTransitions(StateId id) const {
    return states_[static_cast<size_t>(id)].transitions;
  }

  bool isAccepting(StateId s) const {
    return acceptTags_.find(s) != acceptTags_.end();
  }

  Tag acceptTag(StateId s) const {
    if (auto i = acceptTags_.find(s); i != acceptTags_.end())
      return i->second;

    throw std::invalid_argument{"n"};
  }

  std::optional<StateId> delta(StateId state, Symbol symbol) const {
    const auto& T = states_[state].transitions;
    if (auto i = T.find(symbol); i != T.end())
      return i->second;

    return std::nullopt;
  }

 private:
  StateVec states_;
  StateId initialState_;
  AcceptMap acceptTags_;
};

} // namespace klex
