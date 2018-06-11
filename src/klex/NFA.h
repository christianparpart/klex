// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/State.h>
#include <klex/util/UnboxedRange.h>

#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

namespace klex {

class Alphabet;
class DotVisitor;
class DFA;

/**
 * NFA Builder with the Thompson's Construction properties.
 *
 * <ul>
 *   <li> There is exactly one initial state and exactly one accepting state..
 *   <li> No transition other than the initial transition enters the initial state.
 *   <li> The accepting state has no leaving edges
 *   <li> An ε-transition always connects two states that were (earlier in the construction process)
 *        the initial state and the accepting state of NFAs for some component REs.
 *   <li> Each state has at most two entering states and at most two leaving states.
 * </ul>
 */
class NFA {
 public:
  //! holds a vector of State Ids.
  using StateIdVec = std::vector<StateId>;

  //! represent a transition table for a specific state
  using TransitionMap = std::map<Symbol, StateIdVec>;

  //! defines a set of states within one NFA. the index represents the state Id.
  using StateVec = std::vector<TransitionMap>;

  //! defines a map of accepting states with their associated tag. A tag of 0 means "default".
  using AcceptMap = std::map<StateId, Tag>;

  NFA(const NFA& other) = default;
  NFA(NFA&&) = default;
  NFA& operator=(const NFA& other) = default;
  NFA& operator=(NFA&&) = default;

  //! Constructs an empty NFA.
  NFA()
      : states_{},
        initialState_{0},
        acceptState_{0},
        acceptTags_{} {
  }

  /**
   * Constructs an NFA for a single character transition.
   *
   * *No* acceptState flag is set on the accepting node!
   */
  explicit NFA(Symbol value)
      : NFA{} {
    initialState_ = createState();
    acceptState_ = createState();
    addTransition(initialState_, value, acceptState_);
    acceptTags_[acceptState_] = 0; // 0 == DefaultAcceptAction
  }

  void addTransition(StateId from, Symbol s, StateId to) {
    states_[from][s].push_back(to);
  }

  /**
   * Traverses all states and edges in this NFA and calls @p visitor for each state & edge.
   *
   * Use this function to e.g. get a GraphViz dot-file drawn.
   */
  void visit(DotVisitor& visitor) const;

  //! Tests whether or not this is an empty NFA.
  bool empty() const noexcept { return states_.empty(); }

  //! Retrieves the number of states of this NFA.
  size_t size() const noexcept { return states_.size(); }

  //! Retrieves the one and only initial state. This value is nullptr iff the NFA is empty.
  StateId initialStateId() const noexcept { return initialState_; }

  //! Retrieves the one and only accept state. This value is nullptr iff the NFA is empty.
  StateId acceptStateId() const noexcept { return acceptState_; }

  //! Retrieves the list of states this FA contains.
  const StateVec& states() const { return states_; }
  StateVec& states() { return states_; }

  //! Retrieves the alphabet of this finite automaton.
  Alphabet alphabet() const;

  //! Clones this NFA.
  NFA clone() const;

  //! Concatenates the right FA's initial state with this FA's accepting state.
  NFA& concatenate(NFA rhs);

  //! Reconstructs this FA to alternate between this FA and the @p other FA.
  NFA& alternate(NFA other);

  //! Reconstructs this FA to allow optional input. X -> X?
  NFA& optional();

  //! Reconstructs this FA with the given @p quantifier factor.
  NFA& times(unsigned quantifier);

  //! Reconstructs this FA to allow recurring input. X -> X*
  NFA& recurring();

  //! Reconstructs this FA to be recurring at least once. X+ = XX*
  NFA& positive();

  //! Reconstructs this FA to be repeatable between range [minimum, maximum].
  NFA& repeat(unsigned minimum, unsigned maximum);

  //! Retrieves transitions for state with the ID @p id.
  const TransitionMap& stateTransitions(StateId id) const {
    return states_[id];
  }

  TransitionMap& stateTransitions(StateId id) {
    return states_[id];
  }

  //! Flags given state as accepting-state with given Tag @p acceptTag.
  void setAccept(Tag acceptTag) {
    acceptTags_[acceptState_] = acceptTag;
  }

  Tag acceptTag(StateId s) const {
    if (auto i = acceptTags_.find(s); i != acceptTags_.end())
      return i->second;

    return 0;
  }

  bool isAccepting(StateId s) const {
    return acceptTags_.find(s) != acceptTags_.end();
  }

 private:
  StateId createState();
  StateId createState(Tag acceptTag);
  void visit(DotVisitor& v, StateId s, std::unordered_map<StateId, size_t>& registry) const;
  void prepareStateIds(StateId baseId);

 private:
  StateVec states_;
  StateId initialState_;
  StateId acceptState_;
  AcceptMap acceptTags_;
};

} // namespace klex
