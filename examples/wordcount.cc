// This file is part of the "klex" project, http://github.com/christianparpart/klex>
//   (c) 2018 Christian Parpart <christian@parpart.family>
//
// Licensed under the MIT License (the "License"); you may not use this
// file except in compliance with the License. You may obtain a copy of
// the License at: http://opensource.org/licenses/MIT

#include <klex/Compiler.h>
#include <klex/Lexer.h>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

int main(int argc, const char* argv[]) {
  klex::Compiler cc;
  cc.parse(R"(
      Word  ::= [a-zA-Z]+
      LF    ::= \n
      Other ::= .
      Eof   ::= <<EOF>>
  )");

  int words = 0;
  int chars = 0;
  int lines = 0;

  klex::Lexer<int, int, false, false> lexer { cc.compile(), std::cin };
  for (bool eof = false; !eof; ) {
    switch (lexer.recognize()) {
      case 4: // EOF
        eof = true;
        break;
      case 3: // Other
        assert(lexer.token() == 3);
        chars++;
        break;
      case 2: // LF
        chars++;
        lines++;
        break;
      case 1: // Word
        words++;
        chars += lexer.word().size();
        break;
    }
  }

  std::cout << "newlines: " << lines << ", words: " << words << ", characters: " << chars << "\n";

  return EXIT_SUCCESS;
}
