#pragma once

#include <lexer/fa.h>
#include <string_view>

namespace lexer {

class Lexer;

class Builder {
 public:
  void declare(int id, std::string_view pattern);

  fa::FiniteAutomaton buildAutomaton();

  Lexer compile();

 private:
  fa::ThompsonConstruct fa_;
};

} // namespace lexer

