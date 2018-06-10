// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/DFA.h>
#include <klex/DFABuilder.h>
#include <klex/NFA.h>

#include <deque>
#include <sstream>
#include <vector>

namespace klex {

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

struct DFABuilder::TransitionTable { // {{{
  struct Input {
    int configurationNumber;
    Symbol symbol;
    int targetNumber;
  };

  void insert(int q, Symbol c, int t) {
    auto i = std::find_if(transitions.begin(), transitions.end(),
                          [=](const auto& input) {
        return input.first.configurationNumber == q && input.first.symbol == c;
    });
    if (i == transitions.end()) {
      transitions.emplace_back(Input{q, c}, t);
    } else {
      DEBUG("TransitionTable[q{}][{}] = q{}; already present", q, prettySymbol(c), t);
    }
  }

  std::list<std::pair<Input, int>> transitions;
}; // }}}

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

DFA DFABuilder::construct(NFA nfa) {
  std::vector<const State*> q_0 = epsilonClosure({nfa.initialState()});
  DEBUG("q_0 = epsilonClosure({}) = {}", to_string({nfa.initialState()}), q_0);
  std::vector<std::vector<const State*>> Q = {q_0};          // resulting states
  std::deque<std::vector<const State*>> workList = {q_0};
  TransitionTable T;

  DEBUG(" {:<8} | {:<14} | {:<24} | {:<}", "set name", "DFA state", "NFA states", "ε-closures(q, *)");
  DEBUG("{}", "------------------------------------------------------------------------");

  while (!workList.empty()) {
    std::vector<const State*> q = workList.front();    // each set q represents a valid configuration from the NFA
    workList.pop_front();
    const int q_i = configurationNumber(Q, q);

    std::stringstream dbg;
    for (Symbol c : nfa.alphabet()) {
      std::vector<const State*> t = epsilonClosure(delta(q, c));

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

  // map q_i to d_i and flag accepting states
  int q_i = 0;
  for (std::vector<const State*>& q : Q) {
    // d_i represents the corresponding state in the DFA for all states of q from the NFA
    State* d_i = dfa.createState();
    // std::cerr << fmt::format("map q{} to d{} for {} states, {}.\n", q_i, d_i->id(), q.size(), to_string(q, "d"));

    // if q contains an accepting state, then d is an accepting state in the DFA
    if (containsAcceptingState(q)) {
      d_i->setAccept(true);
      Tag tag{};
      if (determineTag(nfa, q, &tag)) {
        // std::cerr << fmt::format("determineTag: q{} tag {} from {}.\n", q_i, tag, q);
        d_i->setTag(tag);
      } else {
        // std::cerr << fmt::format("DFA accepting state {} merged from input states with different tags {}.\n", q_i, to_string(q));
      }
    }

    q_i++;
  }

  // observe mapping from q_i to d_i
  for (const std::pair<TransitionTable::Input, int>& transition: T.transitions) {
    const int q_i = transition.first.configurationNumber;
    const Symbol c = transition.first.symbol;
    const int t_i = transition.second;
    if (t_i != -1) {
      DEBUG("map d{} |--({})--> d{}", q_i, prettySymbol(c), t_i);
      State* q = dfa.findState(q_i);
      State* t = dfa.findState(t_i);

      q->linkTo(c, t);
    }
  }

  // q_0 becomes d_0 (initial state)
  dfa.setInitialState(dfa.findState(0));

  return dfa;
}

void DFABuilder::epsilonClosure(const State* s, std::vector<const State*>* result) {
  if (std::find(result->begin(), result->end(), s) == result->end())
    result->push_back((State*) s);

  for (const Edge& transition : s->transitions()) {
    if (transition.symbol == EpsilonTransition) {
      epsilonClosure(transition.state, result);
    }
  }
}

std::vector<const State*> DFABuilder::epsilonClosure(const std::vector<const State*>& S) {
  std::vector<const State*> result;

  for (const State* s : S)
    epsilonClosure(s, &result);

  return result;
}

void DFABuilder::delta(const State* s, Symbol c, std::vector<const State*>* result) {
  for (const Edge& transition: s->transitions()) {
    if (transition.symbol == EpsilonTransition) {
      delta(transition.state, c, result);
    } else if (transition.symbol == c) {
      result->push_back((State*) transition.state);
    }
  }
}

std::vector<const State*> DFABuilder::delta(const std::vector<const State*>& q, Symbol c) {
  std::vector<const State*> result;
  for (const State* s : q) {
    delta(s, c, &result);
  }
  return result;
}

int DFABuilder::configurationNumber(const std::vector<std::vector<const State*>>& Q, const std::vector<const State*>& t) {
  int i = 0;
  for (const std::vector<const State*>& q_i : Q) {
    if (q_i == t) {
      return i;
    }
    i++;
  }

  return -1;
}

bool DFABuilder::containsAcceptingState(const std::vector<const State*>& Q) {
  for (const State* q : Q)
    if (q->isAccepting())
      return true;

  return false;
}

bool DFABuilder::determineTag(const NFA& fa, std::vector<const State*> q, Tag* tag) {
  // eliminate target-states that originate from epsilon transitions or have no tag set at all
  for (auto i = q.begin(), e = q.end(); i != e; ) {
    const State* s = *i;
    if (fa.isReceivingEpsilon(s) || !s->tag()) {
      i = q.erase(i);
    } else {
      i++;
    }
  }

  if (q.empty()) {
    // fprintf(stderr, "determineTag: all of q was epsiloned\n");
    *tag = 0;
    return false;
  }

  const Tag lowestTag = std::abs((*std::min_element(
      q.begin(), q.end(),
      [](auto x, auto y) { return std::abs(x->tag()) < std::abs(y->tag()); }))->tag());

  // eliminate lower priorities
  for (auto i = q.begin(), e = q.end(); i != e; ) {
    const State* s = *i;
    if (s->tag() != lowestTag) {
      i = q.erase(i);
    } else {
      i++;
    }
  }
  if (q.empty()) {
    // fprintf(stderr, "determineTag: lowest tag found: %d, but no states left?\n", priority);
    *tag = 0;
    return true;
  }

  *tag = (*std::min_element(
        q.begin(),
        q.end(),
        [](auto x, auto y) { return x->tag() < y->tag(); }))->tag();

  return true;
}

} // namespace klex
