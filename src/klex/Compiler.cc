//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Compiler.h>
#include <klex/DFA.h>
#include <klex/DFABuilder.h>
#include <klex/DFAMinimizer.h>
#include <klex/Lexer.h>
#include <klex/LexerDef.h>
#include <klex/NFA.h>
#include <klex/NFABuilder.h>
#include <klex/RegExpr.h>
#include <klex/RegExprParser.h>

#include <iostream>

namespace klex {

void Compiler::declare(Tag tag, std::string_view pattern) {
  std::unique_ptr<RegExpr> expr = RegExprParser{}.parse(pattern);
  declare(tag, *expr);
}

void Compiler::declare(Tag tag, const RegExpr& pattern) {
  NFA nfa = NFABuilder{}.construct(&pattern);
  nfa.setAccept(tag);

  if (fa_.empty()) {
    fa_ = std::move(nfa);
  } else {
    fa_.alternate(std::move(nfa));
  }
}

DFA Compiler::compileDFA() {
  return DFABuilder{std::move(fa_)}.construct();
}

LexerDef Compiler::compile() {
  DFA dfa = DFABuilder{std::move(fa_)}.construct();
  // return compile(dfa);
  DFA dfamin = DFAMinimizer{dfa}.construct();
  return generateTables(dfamin);
  //return generateTables(MinDFABuilder::construct(DFABuilder{}.construct(std::move(fa_))));
  //return generateTables(DFABuilder{}.construct(std::move(fa_)));
}

LexerDef Compiler::generateTables(const DFA& dfa) {
  const Alphabet alphabet = dfa.alphabet();
  TransitionMap transitionMap;

  for (StateId state = 0, sE = dfa.lastState(); state <= sE; ++state) {
    for (Symbol c : alphabet) {
      if (std::optional<StateId> nextState = dfa.delta(state, c); nextState.has_value()) {
        transitionMap.define(state, c, nextState.value());
      }
    }
  }

  std::map<StateId, Tag> acceptStates;
  for (StateId s : dfa.acceptStates())
    acceptStates.emplace(s, *dfa.acceptTag(s));

  return LexerDef{dfa.initialState(), std::move(transitionMap), std::move(acceptStates)};
}

} // namespace klex
