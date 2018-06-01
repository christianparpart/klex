#pragma once

#include <iostream>
#include <string_view>

namespace lexer {

class Lexer {
 public:
  Lexer(CharacterClassTable, TransitionTable);

  void open(std::unique_ptr<std::istream> input);

  // parses a token and returns its ID (or -1 on lexical error)
  int recognize();

  // the underlying lexeme of the currently recognized token
  std::string_view lexeme() const;

 private:
  // ...
};

class Builder {
 public:
  void declare(int id, std::string_view pattern);

  Lexer compile();

 private:
  fa::ThompsonConstruct fa_;
};

} // namespace lexer
