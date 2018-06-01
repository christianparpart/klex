#include <lexer/fa.h>
#include <lexer/regexpr.h>
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

  if (flags.getBool("help")) {
    std::cout << flags.helpText();
    return EXIT_SUCCESS;
  }

  if (flags.parameters().empty()) {
    std::cerr << "Error. No REGEX provided.\n";
    return EXIT_FAILURE;
  }

  const std::string regexprStr = flags.parameters()[0];
  const bool showNfa = flags.getBool("nfa");
  const bool showDfa = flags.getBool("dfa");
  const bool showMfa = flags.getBool("dfa-min") || (!showDfa && !showNfa);

  std::list<lexer::fa::DotGraph> dg;

  std::unique_ptr<lexer::RegExpr> expr = lexer::RegExprParser{}.parse(regexprStr);

  if (flags.getBool("verbose")) {
    std::cerr << fmt::format("RE input str   : {}\n", regexprStr);
    std::cerr << fmt::format("RE AST print   : {}\n", expr->to_string());
  }

  lexer::fa::FiniteAutomaton nfa = lexer::fa::Generator{}.generate(expr.get());
  nfa.renumber();
  if (showNfa)
    dg.emplace_back(lexer::fa::DotGraph{nfa, "n", "NFA"});
  if (flags.getBool("verbose"))
    std::cerr << fmt::format("NFA states     : {}\n", nfa.states().size());

  lexer::fa::FiniteAutomaton dfa;
  if (showDfa || showMfa) {
    dfa = nfa.deterministic();
    if (showDfa) {
      dg.emplace_back(lexer::fa::DotGraph{dfa, "d", "DFA"});
      if (flags.getBool("verbose")) {
        std::cerr << fmt::format("DFA states     : {}\n", dfa.states().size());
      }
    }
  }

  lexer::fa::FiniteAutomaton dfamin;
  if (showMfa) {
    dfamin = dfa.minimize();
    dfamin.renumber();
    dg.emplace_back(lexer::fa::DotGraph{dfamin, "p", "minimal DFA"});
    if (flags.getBool("verbose")) {
      std::cerr << fmt::format("DFA-min states : {}\n", dfamin.states().size());
    }
  }

  std::cout << lexer::fa::dot(dg, regexprStr);

  return EXIT_SUCCESS;
}
