//#include <lexer/util/Flags.h>
#include <lexer/regexpr.h>
#include <lexer/fa.h>

#include <cstdlib>
#include <iostream>
#include <string>

void dump(std::string regexprStr) {
  lexer::RegExprParser rep;
  std::unique_ptr<lexer::RegExpr> expr = rep.parse(regexprStr);
  std::cerr << fmt::format("RE input str   : {}\n", regexprStr);
  std::cerr << fmt::format("RE AST print   : {}\n", expr->to_string());

  lexer::fa::FiniteAutomaton nfa = lexer::fa::Generator{}.generate(expr.get());
  nfa.renumber();
  std::cerr << fmt::format("NFA states     : {}\n", nfa.states().size());
  // std::cout << nfa.dot(expr->to_string(), "n") << "\n";

  lexer::fa::FiniteAutomaton dfa = nfa.deterministic();
  std::cerr << fmt::format("DFA states     : {}\n", dfa.states().size());
  // std::cout << dfa.dot(expr->to_string(), "d") << "\n";

  lexer::fa::FiniteAutomaton dfamin = dfa.minimize();
  dfamin.renumber();
  std::cerr << fmt::format("DFA-min states : {}\n", dfamin.states().size());
  // std::cout << dfamin.dot(expr->to_string(), "p") << "\n";

  std::cout << lexer::fa::dot({
      lexer::fa::DotGraph{nfa, "n", "NFA"},
      lexer::fa::DotGraph{dfa, "d", "DFA"},
      lexer::fa::DotGraph{dfamin, "p", "DFA-min"}});
}

int main(int argc, const char* argv[]) {
  if (argc == 1) {
    //dump("a|b");
    dump("a(b|c)*");
    return EXIT_SUCCESS;
  }

  if (argc == 2) {
    dump(argv[1]);
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
