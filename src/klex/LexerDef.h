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

namespace klex {

// special tags
constexpr Tag EofTag = static_cast<Tag>(1);
constexpr Tag ErrorTag = static_cast<Tag>(2);
constexpr Tag IgnoreTag = static_cast<Tag>(3);
constexpr Tag FirstUserTag = 4;

using AcceptStateMap = std::map<StateId, Tag>;

struct LexerDef {
  StateId initialStateId;
  TransitionMap transitions;
  AcceptStateMap acceptStates;
};

} // namespace klex
