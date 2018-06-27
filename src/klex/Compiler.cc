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

  NFA& fa = fa_["default"];

  if (fa.empty()) {
    fa = std::move(nfa);
  } else {
    fa.alternate(std::move(nfa));
  }

  if (auto i = names_.find(rule.tag); i != names_.end())
    names_[rule.tag] = fmt::format("{}, {}", i->second, rule.name);
  else
    names_[rule.tag] = rule.name;

  rules_.emplace_back(std::move(rule));
}

MultiDFA Compiler::compileDFA(OvershadowMap* overshadows) {
  std::map<std::string, DFA> dfaMap;
  for (const auto& fa : fa_)
    dfaMap[fa.first] = DFABuilder{fa.second.clone()}.construct(overshadows);

  return constructMultiDFA(std::move(dfaMap));
}

MultiDFA Compiler::compileMinimalDFA() {
  return klex::DFAMinimizer{compileDFA()}.construct();
}

LexerDef Compiler::compile() {
  return generateTables(compileMinimalDFA(), std::move(names_));
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
