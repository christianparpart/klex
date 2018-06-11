// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/DFA.h>
#include <klex/DFAMinimizer.h>
#include <klex/State.h>

#include <list>
#include <map>
#include <vector>

namespace klex {

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

DFAMinimizer::DFAMinimizer(const DFA& dfa) : dfa_{dfa} {
}

std::vector<const State*> DFAMinimizer::nonAcceptStates() const {
  std::vector<const State*> result;

  for (const State* s : dfa_.states())
    if (!s->isAccepting())
      result.push_back(s);

  return result;
}

bool DFAMinimizer::containsInitialState(const std::vector<const State*>& S) const {
  for (const State* s : S)
    if (s == dfa_.initialState())
      return true;

  return false;
}

std::list<std::vector<const State*>>::iterator DFAMinimizer::findGroup(const State* s) {
  for (auto i = T.begin(), e = T.end(); i != e; ++i) {
    std::vector<const State*>& group = *i;
    if (group.front()->tag() == s->tag())
      return i;
  }

  return T.end();
}

int DFAMinimizer::partitionId(State* s) const {
  if (s != nullptr) {
    int i = 0;
    for (const std::vector<const State*>& p : P) {
      if (std::find(p.begin(), p.end(), s) != p.end())
        return i;
      else
        i++;
    }
  }
  return -1;
}

std::list<std::vector<const State*>> DFAMinimizer::split(const std::vector<const State*>& S) const {
  DEBUG("split: {}", to_string(S));

  for (Symbol c : dfa_.alphabet()) {
    // if c splits S into s_1 and s_2
    //      that is, phi(s_1, c) and phi(s_2, c) reside in two different p_i's (partitions)
    // then return {s_1, s_2}

    std::map<int /*target partition set*/ , std::vector<const State*> /*source states*/> t_i;
    for (const State* s : S) {
      State* t = s->transition(c);
      int p_i = partitionId(t);
      t_i[p_i].push_back(s);
    }
    if (t_i.size() != 1) {
      DEBUG("  split: on character '{}' into {} sets", (char)c, t_i.size());
      std::list<std::vector<const State*>> result;
      for (const std::pair<int, std::vector<const State*>>& t : t_i) {
        result.emplace_back(std::move(t.second));
      }
      return result;
    }
  }
  return {S};
}

DFA DFAMinimizer::construct() {
  // group all accept states by their tag
  for (const State* s : dfa_.acceptStates()) {
    if (auto group = findGroup(s); group != T.end())
      group->push_back(s);
    else
      T.push_back({s});
  }

  // add another group for all non-accept states
  T.emplace_front(nonAcceptStates());

  while (P != T) {
    std::swap(P, T);
    T.clear();

    for (std::vector<const State*>& p : P) {
      T.splice(T.end(), split(p));
    }
  }

  // -------------------------------------------------------------------------
  DEBUG("minimization terminated with {} unique partition sets", P.size());
  DFA dfamin;

  // instanciate states
  for (const std::vector<const State*>& p : P) {
    const State* s = *p.begin();
    State* q = dfamin.createState();
    q->setAccept(s->isAccepting());
    q->setTag(s->tag());
    DEBUG("Creating p{}: {} {}", p_i, s->isAccepting() ? "accepting" : "rejecting",
                                      containsInitialState(p) ? "initial" : "");
    if (containsInitialState(p)) {
      dfamin.setInitialState(q);
    }
  }

  // setup transitions
  size_t p_i = 0;
  for (const std::vector<const State*>& p : P) {
    const State* s = *p.begin();
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

  return dfamin;
}

} // namespace klex
