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
  NFA nfa = NFABuilder{}.construct(expr.get());
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
  return compile(dfamin);
  //return compile(MinDFABuilder::construct(DFABuilder{}.construct(std::move(fa_))));
  //return compile(DFABuilder{}.construct(std::move(fa_)));
}

LexerDef Compiler::compile(const DFA& dfa) {
  const Alphabet alphabet = dfa.alphabet();
  TransitionMap transitionMap;

  //std::cout << dot({klex::DotGraph{dfa, "n", ""}}, "", true);

  for (const State* state : dfa.states()) {
    //std::cerr << fmt::format("Walking through state {} (with {} links)\n", state->id(), state->transitions().size());
    for (Symbol c : alphabet) {
      if (const State* nextState = state->transition(c); nextState != nullptr) {
        transitionMap.define(state->id(), c, nextState->id());
      }
    }
  }

  std::map<StateId, Tag> acceptStates;
  for (const State* s : dfa.acceptStates())
    acceptStates.emplace(s->id(), s->tag());

  return LexerDef{dfa.initialState()->id(), std::move(transitionMap), std::move(acceptStates)};
}

} // namespace klex
