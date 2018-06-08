// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Lexer.h>
#include <fstream>

#include "token.h" // generated via mklex

extern klex::LexerDef lexerDef; // generated via mklex

int main(int argc, const char* argv[]) {

  klex::Lexer lexer {lexerDef};
  lexer.open(std::make_unique<std::ifstream>(argv[1]));

  for (int t = lexer.recognize(); t > 0; t = lexer.recognize()) {
    std::cerr << fmt::format("[{}-{}]: token {} (\"{}\")\n",
                             lexer.offset().first,
                             lexer.offset().second,
                             t, lexer.word());
  }

  return EXIT_SUCCESS;
}

