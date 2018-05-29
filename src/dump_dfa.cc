#include <lexer/regexpr.h>
#include <lexer/fa.h>

#include <cstdlib>
#include <iostream>
#include <string>

using namespace lexer;

void dump(std::string regexprStr) {
  RegExprParser rep;
  std::unique_ptr<RegExpr> expr = rep.parse(regexprStr);
  fa::FiniteAutomaton nfa = lexer::fa::Generator{"n"}.generate(expr.get());
  nfa.relabel("n");

  fa::FiniteAutomaton dfa = nfa.deterministic();
  std::cout << dfa.dot(expr->to_string()) << "\n";
}

int main(int argc, const char* argv[]) {
  if (argc == 1) {
    dump("a|b");
    return EXIT_SUCCESS;
  }

  if (argc == 2) {
    dump(argv[1]);
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}


