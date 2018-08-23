// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/regular/State.h>
#include <map>
#include <vector>

namespace klex::regular {

using CharCatId = int;

constexpr CharCatId ErrorCharCat = static_cast<CharCatId>(-1);

/**
 * Represents an error-state, such as invalid input character or unexpected EOF.
 */
constexpr StateId ErrorState {808080}; // static_cast<StateId>(-1);

/**
 * Transition mapping API to map the input (currentState, charCat) to (newState).
 */
class TransitionMap {
 public:
  using Container = std::map<StateId, std::map<Symbol, StateId>>;

  TransitionMap()
      : mapping_{} {}

  TransitionMap(Container mapping)
      : mapping_{std::move(mapping)} {}

  /**
   * Defines a new mapping for (currentState, charCat) to (nextState).
   */
  void define(StateId currentState, Symbol charCat, StateId nextState);

  /**
   * Retrieves the next state for the input (currentState, charCat).
   *
   * @returns the transition from (currentState, charCat) to (nextState) or ErrorState if not defined.
   */
  StateId apply(StateId currentState, Symbol charCat) const;

  /**
   * Retrieves a list of all available states.
   */
  std::vector<StateId> states() const;

  /**
   * Retrieves a map of all transitions from given state @p inputState.
   */
  std::map<Symbol, StateId> map(StateId inputState) const;

 private:
  Container mapping_;
};

} // namespace klex::regular

#include <klex/regular/TransitionMap-inl.h>
