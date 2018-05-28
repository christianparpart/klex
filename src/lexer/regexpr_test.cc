#include <lexer/regexpr.h>
#include <lexer/fa.h>

#include <cstdlib>
#include <iostream>
#include <string>

using namespace lexer;

void testRegExpr(std::string regexprStr, std::string inputStr) {
  RegExprParser rep;
  std::unique_ptr<RegExpr> expr = rep.parse(regexprStr);
  std::cout << "INPUT  : " << inputStr << "\n";
  std::cout << "REGEXP : " << regexprStr << "\n";
  std::cout << "STRIFY : " << expr->to_string() << "\n";

  RegExprEvaluator evaluator;
  bool m = evaluator.match(inputStr, expr.get());
  if (m) {
    std::cout << "MATCH  : true, remainder: " << &inputStr[evaluator.offset()] << "\n";
  } else {
    std::cout << "MATCH  : false" << "\n";
  }
}

void testFA(std::string regexprStr) {
  RegExprParser rep;
  std::unique_ptr<RegExpr> expr = rep.parse(regexprStr);

  fa::ThompsonConstruct nfa = lexer::fa::Generator{"n"}.generate(expr.get());
  nfa.relabel("n");
  std::cout << nfa.dot() << "\n";
}

int main(int argc, const char* argv[]) {
  if (argc == 1) {
    testFA("a|b");
    return EXIT_SUCCESS;
  }

  if (argc == 2) {
    testFA(argv[1]);
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
