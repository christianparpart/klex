#pragma once

#include <lexer/fa.h>
#include <lexer/transitionmap.h>
#include <map>

namespace lexer {

struct LexerDef {
  TransitionMap transitions;
  fa::StateId initialStateId;
  std::map<fa::StateId, fa::Tag> acceptStates;
};

} // namespace lexer
