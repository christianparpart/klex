// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/NFA.h>
#include <map>
#include <utility>
#include <vector>

namespace klex {

class DFA;
class State;

class DFABuilder {
 public:
  //! Map of rules that shows which rule is overshadowed by which other rule.
  using OvershadowMap = std::vector<std::pair<Tag, Tag>>;

  DFABuilder(NFA nfa) : nfa_{std::move(nfa)} {}

  /**
   * Constructs a DFA out of the NFA.
   *
   * @param overshadows if not nullptr, it will be used to store semantic information about
   *                    which rule tags have been overshadowed by which.
   */
  DFA construct(OvershadowMap* overshadows = nullptr);

 private:
  struct TransitionTable;

  DFA constructDFA(const std::vector<StateIdVec>& Q,
                   const TransitionTable& T,
                   OvershadowMap* overshadows) const;

  /**
   * Builds a list of states that can be exclusively reached from S via epsilon-transitions.
   */
  void epsilonClosure(StateId s, std::vector<StateId>* result) const;
  std::vector<StateId> epsilonClosure(const std::vector<StateId>& S) const;
  // [[deprecated]] std::vector<StateId> epsilonClosure(StateId S) const;

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
  static int configurationNumber(const std::vector<std::vector<StateId>>& Q,
                                 const std::vector<StateId>& t);

  /**
   * Returns whether or not the StateSet @p Q contains at least one State that is also "accepting".
   */
  bool containsAcceptingState(const std::vector<StateId>& Q) const;

  /**
   * Checks if @p Q contains a state that is flagged as backtracking state in the NFA and returns
   * the target state within the NFA or @c std::nullopt if not a backtracking state.
   */
  std::optional<StateId> containsBacktrackState(const std::vector<StateId>& Q) const;

  /**
   * Determines the tag to use for the deterministic state representing @p q from non-deterministic FA @p fa.
   *
   * @param q the set of states that reflect a single state in the DFA equal to the input FA
   *
   * @returns the determined tag or std::nullopt if none
   */
  std::optional<Tag> determineTag(const StateIdVec& q) const;

 private:
  const NFA nfa_;
  mutable std::map<Tag, Tag> overshadows_;
};

} // namespace klex
