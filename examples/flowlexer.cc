#include <lexer/lexer.h>
#include <fstream>
#include "flowlex.h"

int main(int argc, const char* argv[]) {
  lexer::Lexer lex{myrules, std::ifstream(argv[1])};

  while (lex.recognize() != -1) {
    lex.dump();
  }

  return EXIT_SUCCESS;
}

