// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/DFA.h>
#include <klex/DFAMinimizer.h>
#include <klex/State.h>

#include <map>
#include <sstream>
#include <iostream>
#include <vector>

namespace klex {

#if 0
#define DEBUG(msg, ...) do { std::cerr << fmt::format(msg, __VA_ARGS__) << "\n"; } while (0)
#else
#define DEBUG(msg, ...) do { } while (0)
#endif

DFAMinimizer::DFAMinimizer(const DFA& dfa)
    : dfa_{dfa},
      alphabet_{dfa_.alphabet()} {
}

bool DFAMinimizer::containsInitialState(const StateIdVec& S) const {
  for (StateId s : S)
    if (s == dfa_.initialState())
      return true;

  return false;
}

DFAMinimizer::PartitionVec::iterator DFAMinimizer::findGroup(StateId s) {
  for (auto i = T.begin(), e = T.end(); i != e; ++i) {
    StateIdVec& group = *i;
    if (dfa_.acceptTag(group.front()) == dfa_.acceptTag(s))
      return i;
  }

  return T.end();
}

std::optional<int> DFAMinimizer::partitionId(StateId s) const {
  int i = 0;
  for (const StateIdVec& p : P) {
    if (std::find(p.begin(), p.end(), s) != p.end())
      return i; // P[i] contains s
    else
      i++;
  }
  return std::nullopt;
}

DFAMinimizer::PartitionVec DFAMinimizer::split(const StateIdVec& S) const {
  for (Symbol c : alphabet_) {
    // if c splits S into s_1 and s_2
    //      that is, phi(s_1, c) and phi(s_2, c) reside in two different p_i's (partitions)
    // then return {s_1, s_2}

    std::map<int /*target partition set*/ , StateIdVec /*source states*/> t_i;
    for (StateId s : S) {
      if (const std::optional<StateId> t = dfa_.delta(s, c); t.has_value()) {
        if (std::optional<int> p_i = partitionId(*t); p_i.has_value()) {
          t_i[*p_i].push_back(s);
        } else {
          t_i[-1].push_back(s);
        }
      } else {
        t_i[-1].push_back(s);
      }
    }
    if (t_i.size() > 1) {
      DEBUG("split: {} on character '{}' into {} sets", to_string(S), (char)c, t_i.size());
      PartitionVec result;
      for (const std::pair<int, StateIdVec>& t : t_i) {
        result.emplace_back(std::move(t.second));
        DEBUG(" partition {}: {}", t.first, t.second);
      }
      return result;
    }
  }
  DEBUG("split: no split needed for {}", to_string(S));
  return {S};
}

void DFAMinimizer::dumpGroups(const PartitionVec& T) {
  DEBUG("dumping groups ({})", T.size());
  int groupNr = 0;
  for (const auto& t : T) {
    std::stringstream sstr;
    sstr << "{";
    for (size_t i = 0, e = t.size(); i != e; ++i) {
      if (i) sstr << ", ";
      sstr << "n" << t[i];
    }
    sstr << "}";
    DEBUG("group {}: {}", groupNr, sstr.str());
    groupNr++;
  }
}

DFA DFAMinimizer::construct() {
  // group all accept states by their tag
  for (StateId s : dfa_.acceptStates()) {
    if (auto group = findGroup(s); group != T.end())
      group->push_back(s);
    else
      T.push_back({s});
  }

  // add another group for all non-accept states
  T.emplace_back(dfa_.nonAcceptStates());

  dumpGroups(T);

  while (P != T) {
    std::swap(P, T);
    T.clear();

    for (StateIdVec& p : P)
      for (StateIdVec& s : split(p))
        T.emplace_back(std::move(s));
  }

  return constructFromPartitions(P);
}

DFA DFAMinimizer::constructFromPartitions(const PartitionVec& P) const {
  DEBUG("minimization terminated with {} unique partition sets", P.size());
  DFA dfamin;
  dfamin.createStates(P.size());

  // build remaps table (used as cache for quickly finding DFAmin StateIds from DFA StateIds)
  std::unordered_map<StateId, StateId> remaps;
  StateId p_i = 0;
  for (const StateIdVec& p : P) {
    for (StateId s : p) {
      remaps[s] = p_i;
    }
    p_i++;
  }

  // instanciate states
  p_i = 0;
  for (const StateIdVec& p : P) {
    const StateId s = *p.begin();
    const StateId q = p_i;
    DEBUG("Creating p{}: {} {}", p_i,
                                 dfa_.isAccepting(s) ? "accepting" : "rejecting",
                                 containsInitialState(p) ? "initial" : "");
    if (std::optional<Tag> tag = dfa_.acceptTag(s); tag.has_value())
      dfamin.setAccept(q, *tag);

    if (containsInitialState(p))
      dfamin.setInitialState(q);

    if (std::optional<StateId> bt = containsBacktrackState(p); bt.has_value())
      dfamin.setBacktrack(p_i, remaps[*bt]);

    p_i++;
  }

  // setup transitions
  p_i = 0;
  for (const StateIdVec& p : P) {
    const StateId s = *p.begin();
    for (const std::pair<Symbol, StateId>& transition : dfa_.stateTransitions(s)) {
      if (std::optional<int> t_i = partitionId(transition.second); t_i.has_value()) {
        DEBUG("map p{} --({})--> p{}", p_i, prettySymbol(transition.first), *t_i);
        dfamin.setTransition(p_i, transition.first, *t_i);
      }
    }
    p_i++;
  }

  return dfamin;
}

std::optional<StateId> DFAMinimizer::containsBacktrackState(const std::vector<StateId>& Q) const {
  for (StateId q : Q)
    if (std::optional<StateId> t = dfa_.backtrack(q); t.has_value())
      return *t;

  return std::nullopt;
}

} // namespace klex
