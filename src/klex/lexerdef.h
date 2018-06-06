// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/fa.h>
#include <klex/transitionmap.h>
#include <map>

namespace klex {

struct LexerDef {
  TransitionMap transitions;
  fa::StateId initialStateId;
  std::map<fa::StateId, fa::Tag> acceptStates;
};

} // namespace klex
