// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/State.h>
#include <klex/LexerDef.h>
#include <klex/NFA.h>

#include <string_view>

namespace klex {

class RegExpr;

class Compiler {
 public:
  Compiler() : fa_{} {}

  void declare(Tag tag, std::string_view pattern);
  void declare(Tag tag, const RegExpr& pattern);

  const NFA& nfa() const { return fa_; }

  DFA compileDFA();
  LexerDef compile();

  static LexerDef generateTables(const DFA& dfa);

 private:
  NFA fa_;
};

} // namespace klex

