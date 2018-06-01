#include <lexer/builder.h>
#include <lexer/fa.h>
#include <lexer/util/Flags.h>

#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, const char* argv[]) {
  lexer::util::Flags flags;
  flags.enableParameters("REGEX", "Regular expression");
  flags.defineBool("nfa", 'n', "Prints DFA");
  flags.defineBool("dfa", 'd', "Prints DFA");
  flags.defineBool("dfa-min", 'm', "Prints minimized DFA");
  flags.defineBool("verbose", 'v', "Prints some more verbose output");
  flags.defineBool("help", 'h', "Prints this help and exits");
  flags.parse(argc, argv);

  lexer::Builder builder;
  builder.declare(1, "0|[1-9][0-9]*"); // NUMBER
  builder.declare(2, "if");
  builder.declare(3, "iff");
  builder.declare(4, "then");

  lexer::fa::FiniteAutomaton fa = builder.buildAutomaton();

  std::cout << lexer::fa::dot({lexer::fa::DotGraph{fa, "n", ""}});

  return EXIT_SUCCESS;
}

