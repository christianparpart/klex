// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/DFA.h>
#include <klex/ThompsonConstruct.h>
#include <iostream>
#include <deque>
#include <map>
#include <sstream>
#include <vector>

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

namespace klex {

// {{{ helper
/**
 * Builds a list of states that can be exclusively reached from S via epsilon-transitions.
 */
static StateSet epsilonClosure(const StateSet& S) {
  StateSet result;

  for (State* s : S) {
    result.insert(s);
    for (Edge& transition : s->transitions()) {
      if (transition.symbol == EpsilonTransition) {
        result.merge(epsilonClosure({transition.state}));
      }
    }
  }

  return result;
}

/**
 * Computes a valid configuration the FA can reach with the given input @p q and @p c.
 * 
 * @param q valid input configuration of the original NFA.
 * @param c the input character that the FA would consume next
 *
 * @return set of states that the FA can reach from @p c given the input @p c.
 */
static StateSet delta(const StateSet& q, Symbol c) {
  StateSet result;
  for (State* s : q) {
    for (Edge& transition: s->transitions()) {
      if (transition.symbol == EpsilonTransition) {
        result.merge(delta({transition.state}, c));
      } else if (transition.symbol == c) {
        result.insert(transition.state);
      }
    }
  }
  return result;
}

/**
 * Finds @p t in @p Q and returns its offset (aka configuration number) or -1 if not found.
 */
static int configurationNumber(const std::vector<StateSet>& Q, const StateSet& t) {
  int i = 0;
  for (const StateSet& q_i : Q) {
    if (q_i == t) {
      return i;
    }
    i++;
  }

  return -1;
}

/**
 * Returns whether or not the StateSet @p Q contains at least one State that is also "accepting".
 */
static bool containsAcceptingState(const StateSet& Q) {
  for (State* q : Q)
    if (q->isAccepting())
      return true;

  return false;
}

// }}}

Alphabet DFA::alphabet() const {
  Alphabet alphabet;
  for (const State* state : states()) {
    for (const Edge& transition : state->transitions()) {
      if (transition.symbol != EpsilonTransition) {
        alphabet.insert(transition.symbol);
      }
    }
  }
  return alphabet;
}

StateSet DFA::acceptStates() const {
  StateSet result;

  for (State* s : states())
    if (s->isAccepting())
      result.insert(s);

  return result;
}

State* DFA::findState(StateId id) const {
  auto s = std::find_if(states_.begin(), states_.end(),
                        [id](const auto& s) { return s->id() == id;});
  if (s != states_.end())
    return s->get();

  return nullptr;
}

/**
 * Determines the tag to use for the deterministic state representing @p q from non-deterministic FA @p fa.
 *
 * @param fa the owning finite automaton being operated on
 * @param q the set of states that reflect a single state in the DFA equal to the input FA
 * @param tag address to the Tag the resulting will be stored to
 *
 * @returns whether or not the tag could be determined.
 */
bool determineTag(const ThompsonConstruct& fa, StateSet q, Tag* tag) {
  // eliminate target-states that originate from epsilon transitions or have no tag set at all
  for (auto i = q.begin(), e = q.end(); i != e; ) {
    State* s = *i;
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
    State* s = *i;
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

// --------------------------------------------------------------------------

DFA DFA::minimize() const {
  auto nonAcceptStates = [&]() -> StateSet {
    StateSet result;

    for (State* s : states())
      if (!s->isAccepting())
        result.insert(s);

    return result;
  };

  std::list<StateSet> T = {acceptStates(), nonAcceptStates()};
  std::list<StateSet> P = {};

  auto partitionId = [&](State* s) -> int {
    if (s != nullptr) {
      int i = 0;
      for (const StateSet& p : P) {
        if (p.find(s) != p.end())
          return i;
        else
          i++;
      }
    }
    return -1;
  };

  auto containsInitialState = [this](const StateSet& S) -> bool {
    for (State* s : S)
      if (s == initialState_)
        return true;
    return false;
  };

  auto split = [&](const StateSet& S) -> std::list<StateSet> {
    DEBUG("split: {}", to_string(S));

    for (Symbol c : alphabet()) {
      // if c splits S into s_1 and s_2
      //      that is, phi(s_1, c) and phi(s_2, c) reside in two different p_i's (partitions)
      // then return {s_1, s_2}

      std::map<int /*target partition set*/ , StateSet /*source states*/> t_i;
      for (State* s : S) {
        State* t = s->transition(c);
        int p_i = partitionId(t);
        t_i[p_i].insert(s);
      }
      if (t_i.size() != 1) {
        DEBUG("  split: on character '{}' into {} sets", (char)c, t_i.size());
        std::list<StateSet> result;
        for (const std::pair<int, StateSet>& t : t_i) {
          result.emplace_back(std::move(t.second));
        }
        return result;
      }
    }
    return {S};
  };

  while (P != T) {
    P = std::move(T);
    T = {};

    for (StateSet& p : P) {
      T.splice(T.end(), split(p));
    }
  }

  // -------------------------------------------------------------------------
  DEBUG("minimization terminated with {} unique partition sets", P.size());
  DFA dfamin;

  // instanciate states
  int p_i = 0;
  for (const StateSet& p : P) {
    State* s = *p.begin();
    State* q = dfamin.createState(p_i);
    q->setAccept(s->isAccepting());
    DEBUG("Creating p{}: {} {}", p_i, s->isAccepting() ? "accepting" : "rejecting",
                                      containsInitialState(p) ? "initial" : "");
    if (containsInitialState(p)) {
      dfamin.setInitialState(q);
    }
    p_i++;
  }

  // setup transitions
  p_i = 0;
  for (const StateSet& p : P) {
    State* s = *p.begin();
    State* t0 = dfamin.findState(p_i);
    for (const Edge& transition : s->transitions()) {
      if (int t_i = partitionId(transition.state); t_i != -1) {
        DEBUG("map p{} --({})--> p{}", p_i, prettySymbol(transition.symbol), t_i);
        State* t1 = dfamin.findState(t_i);
        t0->linkTo(transition.symbol, t1);
      }
    }
    p_i++;
  }

  // TODO
  // construct states & links out of P

  return dfamin;
}

std::tuple<OwnedStateSet, State*> DFA::release() {
  std::tuple<OwnedStateSet, State*> result { std::move(states_), initialState_ };

  states_.clear();
  initialState_ = nullptr;

  return result;
}

State* DFA::createState(StateId id) {
  for (State* s : states())
    if (s->id() == id)
      throw std::invalid_argument{fmt::format("StateId: {}", id)};

  return states_.insert(std::make_unique<State>(id)).first->get();
}


void DFA::setInitialState(State* s) {
  // TODO: assert (s is having no predecessors)
  initialState_ = s;
}

struct TransitionTable { // {{{
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

/* {{{ DFA construction visualization
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
}}} */
DFA DFA::construct(ThompsonConstruct nfa) {
  StateSet q_0 = epsilonClosure({nfa.initialState()});
  DEBUG("q_0 = epsilonClosure({}) = {}", to_string({nfa.initialState()}), q_0);
  std::vector<StateSet> Q = {q_0};          // resulting states
  std::deque<StateSet> workList = {q_0};
  TransitionTable T;

  DEBUG(" {:<8} | {:<14} | {:<24} | {:<}", "set name", "DFA state", "NFA states", "ε-closures(q, *)");
  DEBUG("{}", "------------------------------------------------------------------------");

  while (!workList.empty()) {
    StateSet q = workList.front();    // each set q represents a valid configuration from the NFA
    workList.pop_front();
    const int q_i = configurationNumber(Q, q);

    std::stringstream dbg;
    for (Symbol c : nfa.alphabet()) {
      StateSet t = epsilonClosure(delta(q, c));

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
  for (StateSet& q : Q) {
    // d_i represents the corresponding state in the DFA for all states of q from the NFA
    State* d_i = dfa.createState(q_i);
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

} // namespace klex
