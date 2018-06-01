#include <lexer/util/Flags.h>
#include <lexer/regexpr.h>
#include <lexer/fa.h>

#include <cstdlib>
#include <iostream>
#include <string>

void dump(std::string regexprStr) {
}

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

  std::string regexprStr = flags.parameters()[0];

  lexer::RegExprParser rep;
  std::unique_ptr<lexer::RegExpr> expr = rep.parse(regexprStr);
  if (flags.getBool("verbose")) {
    std::cerr << fmt::format("RE input str   : {}\n", regexprStr);
    std::cerr << fmt::format("RE AST print   : {}\n", expr->to_string());
  }

  lexer::fa::FiniteAutomaton nfa = lexer::fa::Generator{}.generate(expr.get());
  nfa.renumber();
  if (flags.getBool("verbose"))
    std::cerr << fmt::format("NFA states     : {}\n", nfa.states().size());

  lexer::fa::FiniteAutomaton dfa = nfa.deterministic();
  if (flags.getBool("verbose"))
    std::cerr << fmt::format("DFA states     : {}\n", dfa.states().size());

  lexer::fa::FiniteAutomaton dfamin = dfa.minimize();
  dfamin.renumber();
  if (flags.getBool("verbose"))
    std::cerr << fmt::format("DFA-min states : {}\n", dfamin.states().size());

  std::list<lexer::fa::DotGraph> dg;
  if (flags.getBool("nfa"))
    dg.emplace_back(lexer::fa::DotGraph{nfa, "n", "NFA"});
  if (flags.getBool("dfa"))
    dg.emplace_back(lexer::fa::DotGraph{dfa, "d", "DFA"});
  if (flags.getBool("dfa-min") || dg.empty())
    dg.emplace_back(lexer::fa::DotGraph{dfamin, "p", "minimal DFA"});

  std::cout << lexer::fa::dot(dg, regexprStr);

  return EXIT_SUCCESS;
}
