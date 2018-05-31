//#include <lexer/util/Flags.h>
#include <lexer/regexpr.h>
#include <lexer/fa.h>

#include <cstdlib>
#include <iostream>
#include <string>

void dump(std::string regexprStr) {
  std::cerr << "RE: " << regexprStr << "\n";
  lexer::RegExprParser rep;
  std::unique_ptr<lexer::RegExpr> expr = rep.parse(regexprStr);
  std::cerr << "RE: " << expr->to_string() << "\n";
  lexer::fa::FiniteAutomaton nfa = lexer::fa::Generator{"n"}.generate(expr.get());
  nfa.relabel("n");
  // std::cout << nfa.dot(expr->to_string()) << "\n";

  lexer::fa::FiniteAutomaton dfa = nfa.deterministic();
  lexer::fa::FiniteAutomaton dfamin = dfa.minimize();
  std::cout << dfamin.dot(expr->to_string()) << "\n";
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
