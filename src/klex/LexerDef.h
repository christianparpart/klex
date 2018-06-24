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
};

} // namespace klex
