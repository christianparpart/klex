// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/State.h>
#include <klex/Rule.h>
#include <klex/LexerDef.h>
#include <klex/NFA.h>

#include <map>
#include <string>
#include <string_view>

namespace klex {

class RegExpr;

class Compiler {
 public:
  Compiler() : fa_{} {}

  void declareAll(const RuleList& rules);
  void declare(const Rule& rule);

  const NFA& nfa() const { return fa_; }
  const std::map<Tag, std::string> names() const noexcept { return names_; }

  DFA compileDFA();
  DFA compileMinimalDFA();
  LexerDef compile();

  static LexerDef generateTables(const DFA& dfa, const std::map<Tag, std::string> names);

 private:
  NFA fa_;
  std::map<Tag, std::string> names_;
};

} // namespace klex

