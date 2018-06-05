#pragma once

#include <lexer/fa.h>
#include <lexer/lexerdef.h>
#include <string_view>

namespace lexer {

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

} // namespace lexer

