#include <lexer/builder.h>
#include <lexer/lexer.h>
#include <lexer/fa.h>
#include <lexer/util/Flags.h>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, const char* argv[]) {
  lexer::util::Flags flags;
  flags.enableParameters("REGEX", "Regular expression");
  flags.defineBool("nfa", 'n', "Prints DFA");
  flags.defineBool("dfa", 'd', "Prints DFA");
  flags.defineBool("dfa-min", 'm', "Prints minimized DFA");
  flags.defineBool("verbose", 'v', "Prints some more verbose output");
  flags.defineBool("help", 'h', "Prints this help and exits");
  flags.defineBool("no-group-edges", 'G', "No grouped edges in dot graph output");
  flags.defineNumber("print-fa", 'p', "NUMBER", "Prints FA (0=none, 1=ThompsonConstruct, 2=DFA, 3=DFA-min)", 0);
  flags.defineString("file", 'f', "FILE", "Input file with lexer rules");
  flags.parse(argc, argv);

  if (flags.getBool("help")) {
    std::cout << flags.helpText();
    return EXIT_SUCCESS;
  }

  lexer::Builder builder;
  for (const std::string& pattern : flags.parameters())
    builder.declare(0, pattern);

  if (flags.parameters().empty()) {
    //builder.declareHelper("IP4_OCTET", "0|[1-9]|[1-9][0-9]|[01][0-9][0-9]|2[0-4][0-9]|25[0-5]");

    builder.declare(0, " "); // " \t\n");
    builder.declare(1, "if");
    builder.declare(2, "then");
    builder.declare(3, "else");
    builder.declare(4, "0|[1-9][0-9]*"); // NUMBER
    //builder.declare(5, "0|[1-9]|[1-9][0-9]|[01][0-9][0-9]|2[0-4][0-9]|25[0-5]"); // IPv4 octet
    //builder.declare(6, "[0-9]|1[0-9]|2[0-9]|3[012]"); // CIDR mask
  }

  if (int n = flags.getNumber("print-fa"); n >= 1 && n <= 3) {
    lexer::fa::FiniteAutomaton fa = builder.buildAutomaton(static_cast<lexer::Builder::Stage>(n));
    fa.renumber();

    std::cout << lexer::fa::dot({lexer::fa::DotGraph{fa, "n", ""}}, "", !flags.getBool("no-group-edges"));
  } else if (std::string filename = flags.getString("file"); !filename.empty()) {
    lexer::Lexer lexer = builder.compile();
    lexer.open(std::make_unique<std::ifstream>(filename));
    for (int t = lexer.recognize(); t != -1; t = lexer.recognize()) {
      switch (t) {
        default:
          fprintf(stderr, "token %d \"%s\"\n", t, lexer.lexeme().c_str());
          break;
      }
    }
  }

  return EXIT_SUCCESS;
}

