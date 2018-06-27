//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/Symbols.h>
#include <klex/State.h>
#include <klex/DFA.h>
#include <map>
#include <string>

namespace klex {

struct MultiDFA {
  using InitialStateMap = std::map<std::string, StateId>;

  InitialStateMap initialStates;
  DFA dfa;
};

MultiDFA constructMultiDFA(std::map<std::string, DFA> many);

} // namespace klex
