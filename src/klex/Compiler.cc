// This file is part of the "klex" project, http://github.com/christianparpart/klex>
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
#include <klex/MultiDFA.h>
#include <klex/NFA.h>
#include <klex/NFABuilder.h>
#include <klex/RegExpr.h>
#include <klex/RegExprParser.h>
#include <klex/Rule.h>
#include <klex/RuleParser.h>

#include <iostream>

namespace klex {

void Compiler::parse(std::unique_ptr<std::istream> stream) {
  declareAll(RuleParser{std::move(stream)}.parseRules());
}

void Compiler::declareAll(RuleList rules) {
  rules_.reserve(rules_.size() + rules.size());

  for (klex::Rule& rule : rules)
    declare(rule);
}

size_t Compiler::size() const {
  size_t result = 0;
  for (const std::pair<const std::string, NFA>& fa : fa_)
    result += fa.second.size();
  return result;
}

void Compiler::declare(Rule rule) {
  std::unique_ptr<RegExpr> re = RegExprParser{}.parse(rule.pattern, rule.line, rule.column);
  NFA nfa = NFABuilder{}.construct(re.get(), rule.tag);

  for (const std::string& condition : rule.conditions) {
    NFA& fa = fa_[condition];

    if (fa.empty()) {
      fa = nfa.clone();
    } else {
      fa.alternate(nfa.clone());
    }
  }

  if (auto i = names_.find(rule.tag); i != names_.end() && i->first != rule.tag)
    names_[rule.tag] = fmt::format("{}, {}", i->second, rule.name);
  else
    names_[rule.tag] = rule.name;

  rules_.emplace_back(std::move(rule));
}

MultiDFA Compiler::compileMultiDFA(OvershadowMap* overshadows) {
  std::map<std::string, DFA> dfaMap;
  for (const auto& fa : fa_)
    dfaMap[fa.first] = DFABuilder{fa.second.clone()}.construct(overshadows);

  return constructMultiDFA(std::move(dfaMap));
}

MultiDFA Compiler::compileMinimalMultiDFA() {
  return klex::DFAMinimizer{compileMultiDFA()}.constructMultiDFA();
}

DFA Compiler::compileDFA(OvershadowMap* overshadows) {
  assert(fa_.size() == 1);
  return DFABuilder{fa_.begin()->second.clone()}.construct(overshadows);
}

DFA Compiler::compileMinimalDFA() {
  return klex::DFAMinimizer{compileDFA()}.constructDFA();
}

LexerDef Compiler::compile() {
  return generateTables(compileMinimalDFA(), std::move(names_));
}

LexerDef Compiler::compileMulti() {
  return generateTables(compileMinimalMultiDFA(), std::move(names_));
}

LexerDef Compiler::generateTables(const DFA& dfa, const std::map<Tag, std::string>& names) {
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

  // TODO: many initial states !
  return LexerDef{{{"INITIAL", dfa.initialState()}}, std::move(transitionMap), std::move(acceptStates), 
                  dfa.backtracking(), std::move(names)};
}

LexerDef Compiler::generateTables(const MultiDFA& multiDFA, const std::map<Tag, std::string>& names) {
  const Alphabet alphabet = multiDFA.dfa.alphabet();
  TransitionMap transitionMap;

  for (StateId state = 0, sE = multiDFA.dfa.lastState(); state <= sE; ++state) {
    for (Symbol c : alphabet) {
      if (std::optional<StateId> nextState = multiDFA.dfa.delta(state, c); nextState.has_value()) {
        transitionMap.define(state, c, nextState.value());
      }
    }
  }

  std::map<StateId, Tag> acceptStates;
  for (StateId s : multiDFA.dfa.acceptStates())
    acceptStates.emplace(s, *multiDFA.dfa.acceptTag(s));

  // TODO: many initial states !
  return LexerDef{multiDFA.initialStates, std::move(transitionMap), std::move(acceptStates), 
                  multiDFA.dfa.backtracking(), std::move(names)};
}

} // namespace klex
