// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/lexer.h>
#include <fstream>
//#include "table.cc"
#include "token.h"

int main(int argc, const char* argv[]) {
  klex::LexerDef ld{
    0,
    std::map<klex::fa::StateId, std::map<klex::fa::Symbol, klex::fa::StateId>> {
      {1, {{'\t', 1}, {'\n', 1}, {' ', 1}}},
      {0, {{'a', 2}, {'b', 4} }},
      {1, {{'=', 47}}},
    },
    klex::AcceptStateMap {
      {   0,   0 }, //
      {   1, -66 }, // Spacing
      {   4, -65 }, // Comment
      {   6,  29 }, // Mod
      {   7,  37 }, // BitAnd
      {   8,  41 }, // RndOpen
      {   9,  42 }, // RndClose
      {  11,  25 }, // Plus
      { 132,  49 }, // Unless
      { 133,  45 }, // Handler
    }
  };
  // klex::TransitionMap t{
  //   R{ 0, X{T{'\t', 1}, T{'\n', 1}, T{' ', 1}}},
  //   R{ 1, X{T{'=', 47}}},
  //   R{ 2, X{T{'.', 88}}},
  //   R{ 3, X{T{'/', 56}}}
  // };

  // lexer::Lexer lex{lexerDef, std::ifstream(argv[1])};

  // while (lex.recognize() != -1) {
  //   lex.dump();
  // }

  return EXIT_SUCCESS;
}

