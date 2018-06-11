// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/NFA.h>
#include <vector>

namespace klex {

class DFA;
class State;

class DFABuilder {
 public:
  DFABuilder(NFA nfa) : nfa_{nfa} {}
  DFA construct();

 private:
  /**
   * Builds a list of states that can be exclusively reached from S via epsilon-transitions.
   */
  void epsilonClosure(StateId s, std::vector<StateId>* result) const;
  std::vector<StateId> epsilonClosure(const std::vector<StateId>& S) const;
  std::vector<StateId> epsilonClosure(StateId S) const;

  /**
   * Computes a valid configuration the FA can reach with the given input @p q and @p c.
   * 
   * @param q valid input configuration of the original NFA.
   * @param c the input character that the FA would consume next
   *
   * @return set of states that the FA can reach from @p c given the input @p c.
   */
  void delta(StateId s, Symbol c, std::vector<StateId>* result) const;
  std::vector<StateId> delta(const std::vector<StateId>& q, Symbol c) const;

  /**
   * Finds @p t in @p Q and returns its offset (aka configuration number) or -1 if not found.
   */
  static int configurationNumber(const std::vector<std::vector<StateId>>& Q, const std::vector<StateId>& t);

  /**
   * Returns whether or not the StateSet @p Q contains at least one State that is also "accepting".
   */
  bool containsAcceptingState(const std::vector<StateId>& Q);

  /**
   * Determines the tag to use for the deterministic state representing @p q from non-deterministic FA @p fa.
   *
   * @param fa the owning finite automaton being operated on
   * @param q the set of states that reflect a single state in the DFA equal to the input FA
   * @param tag address to the Tag the resulting will be stored to
   *
   * @returns whether or not the tag could be determined.
   */
  bool determineTag(std::vector<StateId> q, Tag* tag) const;

  struct TransitionTable;

 private:
  NFA nfa_;
};

} // namespace klex
