// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/regular/Symbols.h>
#include <klex/regular/State.h>
#include <klex/regular/DFA.h>
#include <map>
#include <string>

namespace klex::regular {

struct MultiDFA {
  using InitialStateMap = std::map<std::string, StateId>;

  InitialStateMap initialStates;
  DFA dfa;
};

MultiDFA constructMultiDFA(std::map<std::string, DFA> many);

} // namespace klex::regular
