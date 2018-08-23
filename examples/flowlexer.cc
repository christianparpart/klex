// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <fmt/format.h>
#include <fstream>
#include <iostream>
#include <klex/regular/Lexer.h>

#include "token.h" // generated via mklex

extern klex::regular::LexerDef lexerDef; // generated via mklex

int main(int argc, const char* argv[]) {
  klex::regular::Lexer<Token, Machine> lexer {lexerDef, std::cin};
  if (argc == 2)
    lexer.open(std::make_unique<std::ifstream>(argv[1]));

  do {
    lexer.recognize();
    std::cerr << fmt::format("[{}-{}]: token {} (\"{}\")\n",
                             lexer.offset().first,
                             lexer.offset().second,
                             to_string(lexer.token()), lexer.word());
  } while (lexer.token() != Token::Eof);

  return EXIT_SUCCESS;
}

