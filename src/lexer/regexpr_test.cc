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

int main(int argc, const char* argv[]) {
  if (argc == 1) {
    testRegExpr("a|b", "aRGH");
    return EXIT_SUCCESS;
  }

  if (argc == 2) {
    testRegExpr(argv[1], argv[2]);
    return EXIT_SUCCESS;
  }

  return EXIT_FAILURE;
}
