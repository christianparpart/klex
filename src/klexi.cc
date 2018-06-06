#include <klex/builder.h>
#include <klex/lexer.h>
#include <klex/fa.h>
#include <klex/util/Flags.h>

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

void constructDefaultLexer(klex::Builder& builder) {
  //builder.declareHelper("IP4_OCTET", "0|[1-9]|[1-9][0-9]|[01][0-9][0-9]|2[0-4][0-9]|25[0-5]");

  builder.declare(10, "[ \t\n]+");
  builder.declare(11, "if");
  builder.declare(12, "else");
  builder.declare(13, "then");
  builder.declare(14, "[a-z][a-z]*"); // identifier
  builder.declare(15, "0|[1-9][0-9]*"); // NUMBER

  // builder.declare(14, "is");
  // builder.declare(15, "of");
  // builder.declare(15, "12");
  // builder.declare(51, "!@#$");
  // builder.declare(5, "0|[1-9]|[1-9][0-9]|[01][0-9][0-9]|2[0-4][0-9]|25[0-5]"); // IPv4 octet
  // builder.declare(6, "[0-9]|1[0-9]|2[0-9]|3[012]"); // CIDR mask
}

int main(int argc, const char* argv[]) {
  klex::util::Flags flags;
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

  klex::Builder builder;
  for (const std::string& pattern : flags.parameters())
    builder.declare(42, pattern);

  if (flags.parameters().empty())
    constructDefaultLexer(builder);

  if (int n = flags.getNumber("print-fa"); n >= 1 && n <= 3) {
    klex::fa::FiniteAutomaton fa = builder.buildAutomaton(static_cast<klex::Builder::Stage>(n));
    if (flags.getBool("renumber"))
      fa.renumber();

    std::cout << klex::fa::dot({klex::fa::DotGraph{fa, "n", ""}}, "", !flags.getBool("no-group-edges"));
  }
  
  if (std::string filename = flags.getString("file"); !filename.empty()) {
    klex::Lexer lexer {builder.compile()};
    lexer.open(std::make_unique<std::ifstream>(filename));
    for (int t = lexer.recognize(); t != -1; t = lexer.recognize()) {
      switch (t) {
        default:
          std::cerr << fmt::format("[{}-{}]: token {} (\"{}\")\n",
                                   lexer.offset().first,
                                   lexer.offset().second,
                                   t, lexer.word());
          break;
      }
    }
  }

  return EXIT_SUCCESS;
}

