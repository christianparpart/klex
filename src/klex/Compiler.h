// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT
#pragma once

#include <klex/State.h>
#include <klex/LexerDef.h>
#include <klex/ThompsonConstruct.h>

#include <string_view>

namespace klex {

class Compiler {
 public:
  Compiler() : fa_{} {}

  void ignore(std::string_view pattern); // such as " \t\n" or "#.*$"
  void declare(Tag tag, std::string_view pattern);

  DFA compileDFA();
  LexerDef compile();

  static LexerDef compile(const DFA& dfa);

 private:
  ThompsonConstruct fa_;
};

} // namespace klex

