// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Builder.h>
#include <klex/DFA.h>
#include <klex/Lexer.h>
#include <klex/LexerDef.h>
#include <klex/RegExpr.h>
#include <klex/RegExprExporter.h>
#include <klex/ThompsonConstruct.h>
#include <iostream>

namespace klex {

void Builder::declare(Tag tag, std::string_view pattern) {
  std::unique_ptr<RegExpr> expr = RegExprParser{}.parse(pattern);
  ThompsonConstruct nfa = RegExprExporter{}.construct(expr.get());
  DFA dfa = DFA::construct(std::move(nfa));
  DFA dfamin = dfa.minimize();

  // std::cerr << fmt::format("Builder.declare: prio {}, tag {} RE {}\n", nextPriority_, tag, pattern);
  for (State* s : dfamin.acceptStates()) {
    s->setTag(tag);
    s->setPriority(nextPriority_);
  }

  nextPriority_++;

  ThompsonConstruct tc { std::move(dfamin) };

  if (fa_.empty()) {
    fa_ = std::move(tc);
  } else {
    fa_.alternate(std::move(tc));
  }
}

DFA Builder::compileDFA() {
  return DFA::construct(std::move(fa_));
}

LexerDef Builder::compile() {
  return compile(DFA::construct(std::move(fa_)));
}

LexerDef Builder::compile(const DFA& dfa) {
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
