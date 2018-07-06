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

DFA DFABuilder::construct(OvershadowMap* overshadows) {
  const StateIdVec q_0 = nfa_.epsilonClosure({nfa_.initialStateId()});
  std::vector<StateIdVec> Q = {q_0};          // resulting states
  std::deque<StateIdVec> workList = {q_0};
  TransitionTable T;

  const Alphabet alphabet = nfa_.alphabet();

  StateIdVec eclosure;
  StateIdVec delta;
  while (!workList.empty()) {
    const StateIdVec q = std::move(workList.front());    // each set q represents a valid configuration from the NFA
    workList.pop_front();
    const int q_i = configurationNumber(Q, q);

    for (Symbol c : alphabet) {
      nfa_.epsilonClosure(*nfa_.delta(q, c, &delta), &eclosure);
      if (!eclosure.empty()) {
        if (int t_i = configurationNumber(Q, eclosure); t_i != -1) {
          T.insert(q_i, c, t_i); // T[q][c] = eclosure;
        } else {
          Q.emplace_back(eclosure);
          t_i = Q.size() - 1; // equal to configurationNumber(Q, eclosure);
          T.insert(q_i, c, t_i); // T[q][c] = eclosure;
          workList.emplace_back(std::move(eclosure));
        }
        eclosure.clear();
      }
      delta.clear();
    }
  }

  // Q now contains all the valid configurations and T all transitions between them
  return constructDFA(Q, T, overshadows);
}

DFA DFABuilder::constructDFA(const std::vector<StateIdVec>& Q,
                             const TransitionTable& T,
                             OvershadowMap* overshadows) const {
  DFA dfa;
  dfa.createStates(Q.size());

  // build remaps table (used as cache for quickly finding DFA StateIds from NFA StateIds)
  std::unordered_map<StateId, StateId> remaps;
  StateId q_i = 0;
  for (const StateIdVec& q : Q) {
    for (StateId s : q) {
      remaps[s] = q_i;
    }
    q_i++;
  }

  // map q_i to d_i and flag accepting states
  std::map<Tag, Tag> overshadowing;
  q_i = 0;
  for (const StateIdVec& q : Q) {
    // d_i represents the corresponding state in the DFA for all states of q from the NFA
    const StateId d_i = q_i;
    // std::cerr << fmt::format("map q{} to d{} for {} states, {}.\n", q_i, d_i->id(), q.size(), to_string(q, "d"));

    // if q contains an accepting state, then d is an accepting state in the DFA
    if (nfa_.isAnyAccepting(q)) {
      if (std::optional<Tag> tag = determineTag(q, &overshadowing); tag.has_value()) {
        //DEBUG("determineTag: q{} tag {} from {}.", q_i, *tag, q);
        dfa.setAccept(d_i, *tag);
      } else {
        std::cerr << fmt::format(
            "FIXME?: DFA accepting state {} merged from input states with different tags {}.\n",
            q_i, to_string(q));
        abort();
      }
    }

    if (std::optional<StateId> bt = nfa_.containsBacktrackState(q); bt.has_value()) {
      // TODO: verify: must not contain more than one backtracking mapping
      assert(dfa.isAccepting(d_i));
      dfa.setBacktrack(d_i, remaps[*bt]);
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

  if (overshadows) {
    // check if tag is an acceptor in NFA but not in DFA, hence, it was overshadowed by another rule
    for (const std::pair<StateId, Tag> a : nfa_.acceptMap()) {
      const Tag tag = a.second;
      if (!dfa.isAcceptor(tag)) {
        if (auto i = overshadowing.find(tag); i != overshadowing.end()) {
          overshadows->emplace_back(tag, i->second);
        }
      }
    }
  }

  return dfa;
}

int DFABuilder::configurationNumber(const std::vector<StateIdVec>& Q, const StateIdVec& t) {
  int i = 0;
  for (const StateIdVec& q_i : Q) {
    if (q_i == t) {
      return i;
    }
    i++;
  }

  return -1;
}

std::optional<Tag> DFABuilder::determineTag(const StateIdVec& qn, std::map<Tag, Tag>* overshadows) const {
  std::deque<Tag> tags;

  for (StateId s : qn)
    if (std::optional<Tag> t = nfa_.acceptTag(s); t.has_value())
      tags.push_back(*t);

  if (tags.empty())
    return std::nullopt;

  std::sort(tags.begin(), tags.end());

  std::optional<Tag> lowestTag = tags.front();
  tags.erase(tags.begin());

  for (Tag tag : tags)
    (*overshadows)[tag] = *lowestTag; // {tag} is overshadowed by {lowestTag}

  return lowestTag;
}

} // namespace klex
