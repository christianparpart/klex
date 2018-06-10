// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <set>
#include <utility>
#include <vector>
#include <klex/State.h>
#include <klex/util/UnboxedRange.h>

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
class ThompsonConstruct {
 public:
  ThompsonConstruct(const ThompsonConstruct& other) = delete;
  ThompsonConstruct& operator=(const ThompsonConstruct& other) = delete;

  ThompsonConstruct(ThompsonConstruct&&) = default;
  ThompsonConstruct& operator=(ThompsonConstruct&&) = default;

  //! Constructs an empty NFA.
  ThompsonConstruct()
      : states_{},
        initialState_{nullptr},
        acceptState_{nullptr},
        acceptTag_{0} {
  }

  //! Constructs an NFA for a single character transition
  explicit ThompsonConstruct(Symbol value)
      : ThompsonConstruct{} {
    initialState_ = createState();
    acceptState_ = createState();
    initialState_->linkTo(value, acceptState_);
  }

  /**
   * Traverses all states and edges in this NFA and calls @p visitor for each state & edge.
   *
   * Use this function to e.g. get a GraphViz dot-file drawn.
   */
  void visit(DotVisitor& visitor);

  //! Tests whether or not this is an empty NFA.
  bool empty() const noexcept { return states_.empty(); }

  //! Retrieves the one and only initial state. This value is nullptr iff the NFA is empty.
  State* initialState() const noexcept { return initialState_; }

  //! Retrieves the one and only accept state. This value is nullptr iff the NFA is empty.
  State* acceptState() const noexcept { return acceptState_; }

  //! Retrieves the list of states this FA contains.
  const StateVec& states() const { return states_; }

  //! Retrieves the alphabet of this finite automaton.
  Alphabet alphabet() const;

  //! Clones this NFA.
  ThompsonConstruct clone() const;

  //! Concatenates the right FA's initial state with this FA's accepting state.
  ThompsonConstruct& concatenate(ThompsonConstruct rhs);

  //! Reconstructs this FA to alternate between this FA and the @p other FA.
  ThompsonConstruct& alternate(ThompsonConstruct other);

  //! Reconstructs this FA to allow optional input. X -> X?
  ThompsonConstruct& optional();

  //! Reconstructs this FA with the given @p quantifier factor.
  ThompsonConstruct& times(unsigned quantifier);

  //! Reconstructs this FA to allow recurring input. X -> X*
  ThompsonConstruct& recurring();

  //! Reconstructs this FA to be recurring at least once. X+ = XX*
  ThompsonConstruct& positive();

  //! Reconstructs this FA to be repeatable between range [minimum, maximum].
  ThompsonConstruct& repeat(unsigned minimum, unsigned maximum);

  //! @returns true if @p targetState is only reached via epsilon transition
  bool isReceivingEpsilon(const State* targetState) const noexcept;

 private:
  State* createState();
  State* createState(bool accepting, Tag acceptTag);
  State* createState(StateId id, bool accepting, Tag acceptTag);
  State* findState(StateId id) { return states_.find(id); }

 private:
  StateVec states_;
  State* initialState_;
  State* acceptState_;
  Tag acceptTag_;
};

} // namespace klex
