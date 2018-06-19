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
#include <klex/Rule.h>

#include <iostream>

namespace klex {

void Compiler::declare(const Rule& rule) {
  std::unique_ptr<RegExpr> re = klex::RegExprParser{}.parse(rule.pattern, rule.line, rule.column);
  NFA nfa = NFABuilder{}.construct(re.get());
  nfa.setAccept(rule.tag);

  if (fa_.empty()) {
    fa_ = std::move(nfa);
  } else {
    fa_.alternate(std::move(nfa));
  }

  if (auto i = names_.find(rule.tag); i != names_.end())
    names_[rule.tag] = fmt::format("{}, {}", i->second, rule.name);
  else
    names_[rule.tag] = rule.name;
}

DFA Compiler::compileDFA() {
  return DFABuilder{std::move(fa_)}.construct();
}

LexerDef Compiler::compile() {
  DFA dfa = DFABuilder{std::move(fa_)}.construct();
  // return compile(dfa);
  DFA dfamin = DFAMinimizer{dfa}.construct();
  return generateTables(dfamin, std::move(names_));
  //return generateTables(MinDFABuilder::construct(DFABuilder{}.construct(std::move(fa_))), std::move(names_));
  //return generateTables(DFABuilder{}.construct(std::move(fa_)), std::move(names_));
}

LexerDef Compiler::generateTables(const DFA& dfa, const std::map<Tag, std::string> names) {
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

  return LexerDef{dfa.initialState(), std::move(transitionMap), std::move(acceptStates), std::move(names)};
}

} // namespace klex
