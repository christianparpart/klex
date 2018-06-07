// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/fa.h>
#include <map>
#include <vector>

namespace klex {

using CharCatId = int;

constexpr CharCatId ErrorCharCat = static_cast<CharCatId>(-1);

/**
 * Represents an error-state, such as invalid input character or unexpected EOF.
 */
constexpr fa::StateId ErrorState {808080}; // static_cast<fa::StateId>(-1);

/**
 * Transition mapping API to map the input (currentState, charCat) to (newState).
 */
class TransitionMap {
 public:
  TransitionMap()
      : mapping_{} {}

  TransitionMap(std::map<fa::StateId, std::map<fa::Symbol, fa::StateId>> mapping)
      : mapping_{std::move(mapping)} {}

  /**
   * Defines a new mapping for (currentState, charCat) to (nextState).
   */
  void define(fa::StateId currentState, fa::Symbol charCat, fa::StateId nextState);

  /**
   * Retrieves the next state for the input (currentState, charCat).
   *
   * @returns the transition from (currentState, charCat) to (nextState) or ErrorState if not defined.
   */
  fa::StateId apply(fa::StateId currentState, fa::Symbol charCat) const;

  /**
   * Retrieves a list of all available states.
   */
  std::vector<fa::StateId> states() const;

  /**
   * Retrieves a map of all transitions from given state @p inputState.
   */
  std::map<fa::Symbol, fa::StateId> map(fa::StateId inputState) const;

 private:
  std::map<fa::StateId, std::map<fa::Symbol, fa::StateId>> mapping_;
};

} // namespace klex
