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
  lexer::fa::FiniteAutomaton nfa = lexer::fa::Generator{"n"}.generate(expr.get());
  nfa.relabel("n");

  lexer::fa::FiniteAutomaton dfa = nfa.deterministic();
  std::cout << dfa.dot(expr->to_string()) << "\n";
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


