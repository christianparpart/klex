// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <vector>

namespace klex {

class NFA;
class DFA;
class State;

class DFABuilder {
 public:
  DFA construct(NFA nfa);

 private:
  /**
   * Builds a list of states that can be exclusively reached from S via epsilon-transitions.
   */
  static void epsilonClosure(const State* s, std::vector<const State*>* result);
  static std::vector<const State*> epsilonClosure(const std::vector<const State*>& S);

  /**
   * Computes a valid configuration the FA can reach with the given input @p q and @p c.
   * 
   * @param q valid input configuration of the original NFA.
   * @param c the input character that the FA would consume next
   *
   * @return set of states that the FA can reach from @p c given the input @p c.
   */
  static void delta(const State* s, Symbol c, std::vector<const State*>* result);
  static std::vector<const State*> delta(const std::vector<const State*>& q, Symbol c);

  /**
   * Finds @p t in @p Q and returns its offset (aka configuration number) or -1 if not found.
   */
  static int configurationNumber(const std::vector<std::vector<const State*>>& Q, const std::vector<const State*>& t);

  /**
   * Returns whether or not the StateSet @p Q contains at least one State that is also "accepting".
   */
  static bool containsAcceptingState(const std::vector<const State*>& Q);

  /**
   * Determines the tag to use for the deterministic state representing @p q from non-deterministic FA @p fa.
   *
   * @param fa the owning finite automaton being operated on
   * @param q the set of states that reflect a single state in the DFA equal to the input FA
   * @param tag address to the Tag the resulting will be stored to
   *
   * @returns whether or not the tag could be determined.
   */
  bool determineTag(const NFA& fa, std::vector<const State*> q, Tag* tag);

  struct TransitionTable;
};

} // namespace klex
