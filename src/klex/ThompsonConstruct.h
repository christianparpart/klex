// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/State.h>
#include <klex/util/UnboxedRange.h>

namespace klex {

class Alphabet;
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
      : nextId_{0},
        states_{},
        initialState_{nullptr} {
  }

  //! Constructs an NFA for a single character transition
  explicit ThompsonConstruct(Symbol value)
      : nextId_{0},
        states_{},
        initialState_{createState()} {
    State* acceptState = createState();
    acceptState->setAccept(true);
    initialState_->linkTo(value, acceptState);
  }

  /**
   * Constructs this object with the given input automaton, enforcing Thompson's Construction
   * properties onto it.
   */
  explicit ThompsonConstruct(DFA dfa);

  bool empty() const noexcept { return states_.empty(); }

  State* initialState() const { return initialState_; }
  State* acceptState() const;

  //! Retrieves the alphabet of this finite automaton.
  Alphabet alphabet() const;

  void setTag(Tag tag);

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

  //! Retrieves the list of states this FA contains.
  auto states() const { return util::unbox(states_); }

  //! @returns true if @p targetState is only reached via epsilon transition
  bool isReceivingEpsilon(const State* targetState) const noexcept;

 private:
  State* createState();
  State* createState(StateId id, bool accepting, Tag tag);
  State* findState(StateId id) const;

 private:
  StateId nextId_;
  OwnedStateSet states_;
  State* initialState_;
};

} // namespace klex
