// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/DFA.h>
#include <klex/DFABuilder.h>
#include <klex/NFA.h>
#include <klex/State.h>

#include <deque>
#include <iostream>
#include <sstream>
#include <stack>
#include <vector>

namespace klex {

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

struct DFABuilder::TransitionTable { // {{{
  void insert(int q, Symbol c, int t);
  std::unordered_map<int, std::unordered_map<Symbol, int>> transitions;
};

inline void DFABuilder::TransitionTable::insert(int q, Symbol c, int t) {
	transitions[q][c] = t;
}
// }}}

/* DFA construction visualization
  REGEX:      a(b|c)*

  NFA:        n0 --(a)--> n1 --> n2 -----------------------------------> "n7"
                                  \                                       ^
                                   \---> n3 <------------------------    /
                                         \ \                         \  /
                                          \ \----> n4 --(b)--> n5 --> n6
                                           \                          ^
                                            \----> n8 --(c)--> n9 ---/

  DFA:
                                            <---
              d0 --(a)--> "d1" ----(b)--> "d2"--(b)
                             \             |^
                              \         (c)||(b)
                               \           v|
                                \--(c)--> "d3"--(c)
                                            <---


  TABLE:

    set   | DFA   | NFA                 |
    name  | state | state               | 'a'                 | 'b'                 | 'c'
    --------------------------------------------------------------------------------------------------------
    q0    | d0    | {n0}                | {n1,n2,n3,n4,n7,n8} | -none-              | -none-
    q1    | d1    | {n1,n2,n3,n4,n7,n8} | -none-              | {n3,n4,n5,n6,n7,n8} | {n3,n4,n6,n7,n8,n9}
    q2    | d2    | {n3,n4,n5,n6,n7,n8} | -none-              | q2                  | q3
    q3    | d3    | {n3,n4,n6,n7,n8,n9} | -none-              | q2                  | q3
*/

DFA DFABuilder::construct() {
  StateIdVec q_0 = nfa_.epsilonClosure({nfa_.initialStateId()});
  DEBUG("q_0 = epsilonClosure({}) = {}", to_string(std::vector<StateId>{nfa_.initialStateId()}), q_0);
  std::vector<StateIdVec> Q = {q_0};          // resulting states
  std::deque<StateIdVec> workList = {q_0};
  TransitionTable T;

  DEBUG("Dumping accept map ({}):", nfa_.acceptMap().size());
  for ([[maybe_unused]] const auto& m : nfa_.acceptMap())
    DEBUG(" n{} -> {}", m.first, m.second);

  DEBUG("alphabet = {}", nfa_.alphabet());
  DEBUG(" {:<8} | {:<14} | {:<24} | {:<}", "set name", "DFA state", "NFA states", "ε-closures(q, *)");
  DEBUG("{}", "------------------------------------------------------------------------");

  while (!workList.empty()) {
    StateIdVec q = workList.front();    // each set q represents a valid configuration from the NFA
    workList.pop_front();
    const int q_i = configurationNumber(Q, q);

    std::stringstream dbg;
    for (Symbol c : nfa_.alphabet()) {
      StateIdVec t = nfa_.epsilonClosure(nfa_.delta(q, c));

      int t_i = configurationNumber(Q, t);

      if (!dbg.str().empty()) dbg << ", ";
      if (t_i != -1) {
        dbg << prettySymbol(c) << ": q" << t_i;
      } else if (t.empty()) {
        dbg << prettySymbol(c) << ": none";
      } else {
        Q.push_back(t);
        workList.push_back(t);
        t_i = configurationNumber(Q, t);
        dbg << prettySymbol(c) << ": " << to_string(t);
      }
      T.insert(q_i, c, t_i); // T[q][c] = t;
    }
    DEBUG(" q{:<7} | d{:<13} | {:24} | {}", q_i, q_i, to_string(q), dbg.str());
  }
  // Q now contains all the valid configurations and T all transitions between them

  DFA dfa;
  dfa.createStates(Q.size());

  // map q_i to d_i and flag accepting states
  StateId q_i = 0;
  for (const StateIdVec& q : Q) {
    // d_i represents the corresponding state in the DFA for all states of q from the NFA
    const StateId d_i = q_i;
    // std::cerr << fmt::format("map q{} to d{} for {} states, {}.\n", q_i, d_i->id(), q.size(), to_string(q, "d"));

    // if q contains an accepting state, then d is an accepting state in the DFA
    if (containsAcceptingState(q)) {
      if (std::optional<Tag> tag = determineTag(q); tag.has_value()) {
        //DEBUG("determineTag: q{} tag {} from {}.", q_i, *tag, q);
        dfa.setAccept(d_i, *tag);
      } else {
        std::cerr << fmt::format(
            "FIXME?: DFA accepting state {} merged from input states with different tags {}.\n",
            q_i, to_string(q));
        abort();
      }
    }

    q_i++;
  }

  // observe mapping from q_i to d_i
	for (const std::pair<int, std::unordered_map<Symbol, int>>& t0 : T.transitions) {
    for (const std::pair<Symbol, int>& t1 : t0.second) {
      const int q_i = t0.first;
      const Symbol c = t1.first;
      const int t_i = t1.second;
      if (t_i != -1) {
        DEBUG("map d{} |--({})--> d{}", q_i, prettySymbol(c), t_i);
        dfa.setTransition(q_i, c, t_i);
      }
    }
	}

  // q_0 becomes d_0 (initial state)
  dfa.setInitialState(0);

  return dfa;
}

int DFABuilder::configurationNumber(const std::vector<std::vector<StateId>>& Q, const std::vector<StateId>& t) {
  int i = 0;
  for (const std::vector<StateId>& q_i : Q) {
    if (q_i == t) {
      return i;
    }
    i++;
  }

  return -1;
}

bool DFABuilder::containsAcceptingState(const std::vector<StateId>& Q) {
  for (StateId q : Q)
    if (nfa_.isAccepting(q))
      return true;

  return false;
}

std::optional<Tag> DFABuilder::determineTag(const StateIdVec& qn) const {
  std::optional<Tag> lowestTag{};

  for (StateId s : qn) {
    std::optional<Tag> t = nfa_.acceptTag(s);
    if (!t.has_value()) {
      DEBUG("determineTag: n{} is non-accepting", s);
    } else if (!lowestTag || *t < lowestTag) {
      DEBUG("determineTag: possible tag from n{}: {}", s, *t);
      lowestTag = *t;
    } else {
      DEBUG("determineTag: weird n{}: {}", s, *t);
    }
  }

  return *lowestTag;
}

} // namespace klex
