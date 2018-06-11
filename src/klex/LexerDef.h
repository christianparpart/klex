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

using AcceptStateMap = std::map<StateId, Tag>;

struct LexerDef {
  StateId initialStateId;
  TransitionMap transitions;
  AcceptStateMap acceptStates;
};

} // namespace klex