#pragma once

#include <lexer/fa.h>
#include <string_view>

namespace lexer {

class Lexer;

class Builder {
 public:
  void declare(int id, std::string_view pattern);

  enum class Stage {
    ThompsonConstruct = 1,
    Deterministic,
    Minimized,
  };
  fa::FiniteAutomaton buildAutomaton(Stage stage);

  Lexer compile();

 private:
  fa::ThompsonConstruct fa_;
};

} // namespace lexer

