// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#pragma once

#include <klex/fa.h>
#include <klex/lexerdef.h>
#include <string_view>

namespace klex {

class Builder {
 public:
  Builder() : fa_{}, nextPriority_{1} {}

  void ignore(std::string_view pattern); // such as " \t\n" or "#.*$"
  void declare(fa::Tag tag, std::string_view pattern);

  enum class Stage {
    ThompsonConstruct = 1,
    Deterministic,
    Minimized,
  };
  fa::FiniteAutomaton buildAutomaton(Stage stage);

  LexerDef compile();

 private:
  fa::ThompsonConstruct fa_;
  int nextPriority_;
};

} // namespace klex

