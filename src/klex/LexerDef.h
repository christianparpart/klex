// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/TransitionMap.h>
#include <klex/State.h>
#include <map>
#include <string>
#include <sstream>

namespace klex {

// special tags
constexpr Tag IgnoreTag = static_cast<Tag>(-1);
constexpr Tag FirstUserTag = 1;

using AcceptStateMap = std::map<StateId, Tag>;

//! defines a mapping between accept state ID and another (prior) ID to track roll back the input stream to.
using BacktrackingMap = std::map<StateId, StateId>;

struct LexerDef {
  StateId initialStateId;
  TransitionMap transitions;
  AcceptStateMap acceptStates;
  BacktrackingMap backtrackingStates;
  std::map<Tag, std::string> tagNames;

  std::string to_string() const;

  std::string tagName(Tag t) const {
    if (auto i = tagNames.find(t); i != tagNames.end())
      return i->second;

    return std::string();
  }
};

inline std::string LexerDef::to_string() const {
  std::stringstream sstr;

  sstr << fmt::format("initializerState: n{}\n", initialStateId);
  sstr << fmt::format("totalStates: {}\n", transitions.states().size());

  sstr << "transitions:\n";
  for (StateId inputState : transitions.states())
    for (const std::pair<Symbol, StateId>& p : transitions.map(inputState))
      sstr << fmt::format("- n{} --({})--> n{}\n", inputState, prettySymbol(p.first), p.second);

  sstr << "accepts:\n";
  for (const std::pair<StateId, Tag> a : acceptStates)
    sstr << fmt::format("- n{} to {} ({})\n", a.first, a.second, tagName(a.second));

  sstr << "backtracking:\n";
  for (const std::pair<StateId, StateId> bt : backtrackingStates)
    sstr << fmt::format("- n{} to n{}\n", bt.first, bt.second);

  return sstr.str();
}

} // namespace klex
