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
